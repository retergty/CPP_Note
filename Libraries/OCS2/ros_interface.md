# ros interface

本节总结`ocs2`的`ros2`接口

## MPC_ROS_Interface类

`MPC_ROS_Interface`类直接操作`MPC`，运行最优控制问题，同时收发`ROS2`消息。

### 成员变量

```CPP
MPC_BASE& mpc_;
```

所有求解器的基类，保存`mpc`求解器.

```CPP
std::string topicPrefix_;

rclcpp::Node::SharedPtr node_;

// Publishers and subscribers
rclcpp::Subscription<ocs2_msgs::msg::MpcObservation>::SharedPtr
    mpcObservationSubscriber_;
rclcpp::Subscription<ocs2_msgs::msg::MpcTargetTrajectories>::SharedPtr
    mpcTargetTrajectoriesSubscriber_;
rclcpp::Publisher<ocs2_msgs::msg::MpcFlattenedController>::SharedPtr
    mpcPolicyPublisher_;
rclcpp::Service<ocs2_msgs::srv::Reset>::SharedPtr mpcResetServiceServer_;
```

`ROS2`接口，发布订阅话题。

订阅`${topicPrefix_}_mpc_observation`接收当前控制系统的观测值,在回调函数中运行`MPC`.

发布`${topicPrefix_}_mpc_policy`获取`MPC`最优化后的结果，发布控制器指令.

发布`${topicPrefix_}_mpc_reset`服务，用于重置`MPC`.

```CPP
std::unique_ptr<CommandData> bufferCommandPtr_;
std::unique_ptr<CommandData> publisherCommandPtr_;
std::unique_ptr<PrimalSolution> bufferPrimalSolutionPtr_;
std::unique_ptr<PrimalSolution> publisherPrimalSolutionPtr_;
std::unique_ptr<PerformanceIndex> bufferPerformanceIndicesPtr_;
std::unique_ptr<PerformanceIndex> publisherPerformanceIndicesPtr_;
```

保存`MPC`解等数据.

### 构造函数

```CPP
MPC_ROS_Interface::MPC_ROS_Interface(MPC_BASE& mpc, std::string topicPrefix)
    : mpc_(mpc),
      topicPrefix_(std::move(topicPrefix)),
      bufferPrimalSolutionPtr_(new PrimalSolution()),
      publisherPrimalSolutionPtr_(new PrimalSolution()),
      bufferCommandPtr_(new CommandData()),
      publisherCommandPtr_(new CommandData()),
      bufferPerformanceIndicesPtr_(new PerformanceIndex),
      publisherPerformanceIndicesPtr_(new PerformanceIndex) {
  // start thread for publishing
#ifdef PUBLISH_THREAD
  publisherWorker_ = std::thread(&MPC_ROS_Interface::publisherWorker, this);
#endif
}
```

构造类，同时如果定义了`PUBLISH_THREAD`则在另一个线程里发布`MPC`最优化结果.

### launchNodes

```CPP
void MPC_ROS_Interface::launchNodes(const rclcpp::Node::SharedPtr& node) {
  RCLCPP_INFO(LOGGER, "MPC node is setting up ...");
  node_ = node;

  // Observation subscriber
  mpcObservationSubscriber_ =
      node_->create_subscription<ocs2_msgs::msg::MpcObservation>(
          topicPrefix_ + "_mpc_observation", 1,
          std::bind(&MPC_ROS_Interface::mpcObservationCallback, this,
                    std::placeholders::_1));

  // MPC publisher
  mpcPolicyPublisher_ =
      node_->create_publisher<ocs2_msgs::msg::MpcFlattenedController>(
          topicPrefix_ + "_mpc_policy", 1);

  // MPC reset service server
  mpcResetServiceServer_ = node_->create_service<ocs2_msgs::srv::Reset>(
      topicPrefix_ + "_mpc_reset",
      [this](const std::shared_ptr<ocs2_msgs::srv::Reset::Request>& request,
             const std::shared_ptr<ocs2_msgs::srv::Reset::Response>& response) {
        return resetMpcCallback(request, response);
      });

  // display
#ifdef PUBLISH_THREAD
  RCLCPP_INFO(LOGGER, "Publishing SLQ-MPC messages on a separate thread.");
#endif

  RCLCPP_INFO(LOGGER, "MPC node is ready.");

  // spin
  spin();
}
```

启动节点.订阅服务.

### mpcObservationCallback

```CPP
void MPC_ROS_Interface::mpcObservationCallback(
    const ocs2_msgs::msg::MpcObservation::ConstSharedPtr& msg) {
  std::lock_guard<std::mutex> resetLock(resetMutex_);

  if (!resetRequestedEver_.load()) {
    RCLCPP_WARN_STREAM(LOGGER,
                       "MPC should be reset first. Either call "
                       "MPC_ROS_Interface::reset() or use the reset service.");
    return;
  }

  // current time, state, input, and subsystem
  const auto currentObservation = ros_msg_conversions::readObservationMsg(*msg);

  // measure the delay in running MPC
  mpcTimer_.startTimer();

  // run MPC
  bool controllerIsUpdated =
      mpc_.run(currentObservation.time, currentObservation.state);
  if (!controllerIsUpdated) {
    return;
  }
  copyToBuffer(currentObservation);

  // measure the delay for sending ROS messages
  mpcTimer_.endTimer();

  // check MPC delay and solution window compatibility
  scalar_t timeWindow = mpc_.settings().solutionTimeWindow_;
  if (mpc_.settings().solutionTimeWindow_ < 0) {
    timeWindow = mpc_.getSolverPtr()->getFinalTime() - currentObservation.time;
  }
  if (timeWindow < 2.0 * mpcTimer_.getAverageInMilliseconds() * 1e-3) {
    std::cerr << "WARNING: The solution time window might be shorter than the "
                 "MPC delay!\n";
  }

  // display
  if (mpc_.settings().debugPrint_) {
    std::cerr << '\n';
    std::cerr << "\n### MPC_ROS Benchmarking";
    std::cerr << "\n###   Maximum : "
              << mpcTimer_.getMaxIntervalInMilliseconds() << "[ms].";
    std::cerr << "\n###   Average : " << mpcTimer_.getAverageInMilliseconds()
              << "[ms].";
    std::cerr << "\n###   Latest  : "
              << mpcTimer_.getLastIntervalInMilliseconds() << "[ms]."
              << std::endl;
  }

#ifdef PUBLISH_THREAD
  std::unique_lock<std::mutex> lk(publisherMutex_);
  readyToPublish_ = true;
  lk.unlock();
  msgReady_.notify_one();

#else
  ocs2_msgs::msg::MpcFlattenedController mpcPolicyMsg =
      createMpcPolicyMsg(*bufferPrimalSolutionPtr_, *bufferCommandPtr_,
                         *bufferPerformanceIndicesPtr_);
  mpcPolicyPublisher_.publish(mpcPolicyMsg);
#endif
}
```

这是回调函数，接受当前状态，传递给`MPC`进行最优化.

```msg
# MPC observation
float64        time        # Current time
MpcState      state       # Current state
MpcInput      input       # Current input
int8           mode        # Current mode
```

```CPP
// run MPC
bool controllerIsUpdated =
    mpc_.run(currentObservation.time, currentObservation.state);
if (!controllerIsUpdated) {
    return;
}
```

运行`MPC`，调用`mpc_.run`方法.

```CPP
void MPC_ROS_Interface::copyToBuffer(
    const SystemObservation& mpcInitObservation) {
  // buffer policy mutex
  std::lock_guard<std::mutex> policyBufferLock(bufferMutex_);

  // get solution
  scalar_t finalTime =
      mpcInitObservation.time + mpc_.settings().solutionTimeWindow_;
  if (mpc_.settings().solutionTimeWindow_ < 0) {
    finalTime = mpc_.getSolverPtr()->getFinalTime();
  }
  mpc_.getSolverPtr()->getPrimalSolution(finalTime,
                                         bufferPrimalSolutionPtr_.get());

  // command
  bufferCommandPtr_->mpcInitObservation_ = mpcInitObservation;
  bufferCommandPtr_->mpcTargetTrajectories_ =
      mpc_.getSolverPtr()->getReferenceManager().getTargetTrajectories();

  // performance indices
  *bufferPerformanceIndicesPtr_ = mpc_.getSolverPtr()->getPerformanceIndeces();
}
```

获取优化的全部时间，获取所有的`MPC`解.存储到`bufferPrimalSolutionPtr_`中.

获取期望轨迹，当前观测，存储到`bufferCommandPtr_`中,这是传给`MPC`的命令.

### publisherWorker

```CPP
void MPC_ROS_Interface::publisherWorker() {
  while (!terminateThread_) {
    std::unique_lock<std::mutex> lk(publisherMutex_);

    msgReady_.wait(lk, [&] { return (readyToPublish_ || terminateThread_); });

    if (terminateThread_) {
      break;
    }

    {
      std::lock_guard<std::mutex> policyBufferLock(bufferMutex_);
      publisherCommandPtr_.swap(bufferCommandPtr_);
      publisherPrimalSolutionPtr_.swap(bufferPrimalSolutionPtr_);
      publisherPerformanceIndicesPtr_.swap(bufferPerformanceIndicesPtr_);
    }

    ocs2_msgs::msg::MpcFlattenedController mpcPolicyMsg =
        createMpcPolicyMsg(*publisherPrimalSolutionPtr_, *publisherCommandPtr_,
                           *publisherPerformanceIndicesPtr_);

    // publish the message
    mpcPolicyPublisher_->publish(mpcPolicyMsg);

    readyToPublish_ = false;
    lk.unlock();
    msgReady_.notify_one();
  }
}
```

获取`MPC`最优解，最终将`mpc_policy`发送出去.

```msg
# MpcFlattenedController.msg
# Flattened controller: A serialized controller

# define controllerType Enum values
uint8 CONTROLLER_UNKNOWN=0 # safety mechanism: message initalization to zero
uint8 CONTROLLER_FEEDFORWARD=1
uint8 CONTROLLER_LINEAR=2

uint8                   controller_type         # what type of controller is this

MpcObservation          init_observation        # plan initial observation

MpcTargetTrajectories   plan_target_trajectories # target trajectory in cost function
MpcState[]              state_trajectory        # optimized state trajectory from planner
MpcInput[]              input_trajectory        # optimized input trajectory from planner
float64[]               time_trajectory         # time trajectory for stateTrajectory and inputTrajectory
uint16[]                post_event_indices       # array of indices indicating the index of post-event time in the trajectories
ModeSchedule           mode_schedule           # optimal/predefined MPC mode sequence and event times

ControllerData[]       data                   # the actual payload from flatten method: one vector of data per time step

MpcPerformanceIndices performance_indices     # solver performance indices
```

```msg
# ControllerData.sg
float32[] data
```

```CPP
ocs2_msgs::msg::MpcFlattenedController MPC_ROS_Interface::createMpcPolicyMsg(
    const PrimalSolution& primalSolution, const CommandData& commandData,
    const PerformanceIndex& performanceIndices) {
  ocs2_msgs::msg::MpcFlattenedController mpcPolicyMsg;

  mpcPolicyMsg.init_observation = ros_msg_conversions::createObservationMsg(
      commandData.mpcInitObservation_);
  mpcPolicyMsg.plan_target_trajectories =
      ros_msg_conversions::createTargetTrajectoriesMsg(
          commandData.mpcTargetTrajectories_);
  mpcPolicyMsg.mode_schedule =
      ros_msg_conversions::createModeScheduleMsg(primalSolution.modeSchedule_);
  mpcPolicyMsg.performance_indices =
      ros_msg_conversions::createPerformanceIndicesMsg(
          commandData.mpcInitObservation_.time, performanceIndices);

  switch (primalSolution.controllerPtr_->getType()) {
    case ControllerType::FEEDFORWARD:
      mpcPolicyMsg.controller_type =
          ocs2_msgs::msg::MpcFlattenedController::CONTROLLER_FEEDFORWARD;
      break;
    case ControllerType::LINEAR:
      mpcPolicyMsg.controller_type =
          ocs2_msgs::msg::MpcFlattenedController::CONTROLLER_LINEAR;
      break;
    default:
      throw std::runtime_error(
          "MPC_ROS_Interface::createMpcPolicyMsg: Unknown ControllerType");
  }

  // maximum length of the message
  const size_t N = primalSolution.timeTrajectory_.size();

  mpcPolicyMsg.time_trajectory.clear();
  mpcPolicyMsg.time_trajectory.reserve(N);
  mpcPolicyMsg.state_trajectory.clear();
  mpcPolicyMsg.state_trajectory.reserve(N);
  mpcPolicyMsg.data.clear();
  mpcPolicyMsg.data.reserve(N);
  mpcPolicyMsg.post_event_indices.clear();
  mpcPolicyMsg.post_event_indices.reserve(
      primalSolution.postEventIndices_.size());

  // time
  for (auto t : primalSolution.timeTrajectory_) {
    mpcPolicyMsg.time_trajectory.emplace_back(t);
  }

  // post-event indices
  for (auto ind : primalSolution.postEventIndices_) {
    mpcPolicyMsg.post_event_indices.emplace_back(static_cast<uint16_t>(ind));
  }

  // state
  for (size_t k = 0; k < N; k++) {
    ocs2_msgs::msg::MpcState mpcState;
    mpcState.value.resize(primalSolution.stateTrajectory_[k].rows());
    for (size_t j = 0; j < primalSolution.stateTrajectory_[k].rows(); j++) {
      mpcState.value[j] = primalSolution.stateTrajectory_[k](j);
    }
    mpcPolicyMsg.state_trajectory.emplace_back(mpcState);
  }  // end of k loop

  // input
  for (size_t k = 0; k < N; k++) {
    ocs2_msgs::msg::MpcInput mpcInput;
    mpcInput.value.resize(primalSolution.inputTrajectory_[k].rows());
    for (size_t j = 0; j < primalSolution.inputTrajectory_[k].rows(); j++) {
      mpcInput.value[j] = primalSolution.inputTrajectory_[k](j);
    }
    mpcPolicyMsg.input_trajectory.emplace_back(mpcInput);
  }  // end of k loop

  // controller
  scalar_array_t timeTrajectoryTruncated;
  std::vector<std::vector<float>*> policyMsgDataPointers;
  policyMsgDataPointers.reserve(N);
  for (auto t : primalSolution.timeTrajectory_) {
    mpcPolicyMsg.data.emplace_back(ocs2_msgs::msg::ControllerData());

    policyMsgDataPointers.push_back(&mpcPolicyMsg.data.back().data);
    timeTrajectoryTruncated.push_back(t);
  }  // end of k loop

  // serialize controller into data buffer
  primalSolution.controllerPtr_->flatten(timeTrajectoryTruncated,
                                         policyMsgDataPointers);

  return mpcPolicyMsg;
}
```

执行从`PrimalSolution`,`CommandData`,`PerformanceIndex`到`mpcPolicyMsg`的转换.

## MRT_ROS_Interface

`MRT_ROS_Interface`和`MPC_ROS_Interface`协同工作，共同完成`MPC`任务。`MRT_ROS_Interface`提供了一系列的方法，这些方法可以安全地给`MPC_ROS_Interface`传递信息。比如设置`MPC`观测值，获取`MPC`控制规则(policy).

`MRT_ROS_Interface`基类有一系列长度为1的`Buffer`，用于保存最新一次设置的状态

添加这个模块而不是直接使用`ROS2`消息控制`MPC_ROS_Interface`的原因是，还有一个`MPC_MRT_Interface`的非`ROS2`类，为了保持`API`的一致性.

### 成员变量

```CPP
std::string topicPrefix_;

// Publishers and subscribers
rclcpp::Node::SharedPtr node_;
rclcpp::Publisher<ocs2_msgs::msg::MpcObservation>::SharedPtr
    mpcObservationPublisher_;
rclcpp::Subscription<ocs2_msgs::msg::MpcFlattenedController>::SharedPtr
    mpcPolicySubscriber_;
rclcpp::Client<ocs2_msgs::srv::Reset>::SharedPtr mpcResetServiceClient_;

// ROS messages
ocs2_msgs::msg::MpcObservation mpcObservationMsg_;
ocs2_msgs::msg::MpcObservation mpcObservationMsgBuffer_;
```

`ROS2`接口

发布`MpcObservation`到`${topicPrefix_}_mpc_observation`,新的状态测量，发布给`MPC_ROS_Interface`。

从`${topicPrefix_}_mpc_policy`订阅`MpcFlattenedController`,这是`MPC_ROS_Interface`当前最优化的控制结果。

向`${topicPrefix_}_mpc_reset`传输`MPC`重置请求.

### 发布MpcObservation

```CPP
void MRT_ROS_Interface::setCurrentObservation(
    const SystemObservation& currentObservation) {
#ifdef PUBLISH_THREAD
  std::unique_lock<std::mutex> lk(publisherMutex_);
#endif

  // create the message
  mpcObservationMsg_ =
      ros_msg_conversions::createObservationMsg(currentObservation);

  // publish the current observation
#ifdef PUBLISH_THREAD
  readyToPublish_ = true;
  lk.unlock();
  msgReady_.notify_one();
#else
  mpcObservationPublisher_.publish(mpcObservationMsg_);
#endif
}
```

```CPP
void MRT_ROS_Interface::publisherWorkerThread() {
  while (!terminateThread_) {
    std::unique_lock<std::mutex> lk(publisherMutex_);

    msgReady_.wait(lk, [&] { return (readyToPublish_ || terminateThread_); });

    if (terminateThread_) {
      break;
    }

    mpcObservationMsgBuffer_ = std::move(mpcObservationMsg_);

    readyToPublish_ = false;

    lk.unlock();
    msgReady_.notify_one();

    mpcObservationPublisher_->publish(mpcObservationMsgBuffer_);
  }
}
```

将传给的`MpcObservation`保存到`Buffer`里，同时发送给`MPC_ROS_Interface`.

### 接收MPC控制策略

```CPP
void MRT_ROS_Interface::mpcPolicyCallback(
    const ocs2_msgs::msg::MpcFlattenedController::ConstSharedPtr& msg) {
  // read new policy and command from msg
  auto commandPtr = std::make_unique<CommandData>();
  auto primalSolutionPtr = std::make_unique<PrimalSolution>();
  auto performanceIndicesPtr = std::make_unique<PerformanceIndex>();
  readPolicyMsg(*msg, *commandPtr, *primalSolutionPtr, *performanceIndicesPtr);

  this->moveToBuffer(std::move(commandPtr), std::move(primalSolutionPtr),
                     std::move(performanceIndicesPtr));
}
```

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void MRT_ROS_Interface::readPolicyMsg(
    const ocs2_msgs::msg::MpcFlattenedController& msg, CommandData& commandData,
    PrimalSolution& primalSolution, PerformanceIndex& performanceIndices) {
  commandData.mpcInitObservation_ =
      ros_msg_conversions::readObservationMsg(msg.init_observation);
  commandData.mpcTargetTrajectories_ =
      ros_msg_conversions::readTargetTrajectoriesMsg(
          msg.plan_target_trajectories);
  performanceIndices =
      ros_msg_conversions::readPerformanceIndicesMsg(msg.performance_indices);

  const size_t N = msg.time_trajectory.size();
  if (N == 0) {
    throw std::runtime_error(
        "[MRT_ROS_Interface::readPolicyMsg] controller message is empty!");
  }
  if (msg.state_trajectory.size() != N && msg.input_trajectory.size() != N) {
    throw std::runtime_error(
        "[MRT_ROS_Interface::readPolicyMsg] state and input trajectories must "
        "have same length!");
  }
  if (msg.data.size() != N) {
    throw std::runtime_error(
        "[MRT_ROS_Interface::readPolicyMsg] Data has the wrong length!");
  }

  primalSolution.clear();

  primalSolution.modeSchedule_ =
      ros_msg_conversions::readModeScheduleMsg(msg.mode_schedule);

  size_array_t stateDim(N);
  size_array_t inputDim(N);
  primalSolution.timeTrajectory_.reserve(N);
  primalSolution.stateTrajectory_.reserve(N);
  primalSolution.inputTrajectory_.reserve(N);
  for (size_t i = 0; i < N; i++) {
    stateDim[i] = msg.state_trajectory[i].value.size();
    inputDim[i] = msg.input_trajectory[i].value.size();
    primalSolution.timeTrajectory_.emplace_back(msg.time_trajectory[i]);
    primalSolution.stateTrajectory_.emplace_back(
        Eigen::Map<const Eigen::VectorXf>(msg.state_trajectory[i].value.data(),
                                          stateDim[i])
            .cast<scalar_t>());
    primalSolution.inputTrajectory_.emplace_back(
        Eigen::Map<const Eigen::VectorXf>(msg.input_trajectory[i].value.data(),
                                          inputDim[i])
            .cast<scalar_t>());
  }

  primalSolution.postEventIndices_.reserve(msg.post_event_indices.size());
  for (auto ind : msg.post_event_indices) {
    primalSolution.postEventIndices_.emplace_back(static_cast<size_t>(ind));
  }

  std::vector<std::vector<float> const*> controllerDataPtrArray(N, nullptr);
  for (int i = 0; i < N; i++) {
    controllerDataPtrArray[i] = &(msg.data[i].data);
  }

  // instantiate the correct controller
  switch (msg.controller_type) {
    case ocs2_msgs::msg::MpcFlattenedController::CONTROLLER_FEEDFORWARD: {
      auto controller = FeedforwardController::unFlatten(
          primalSolution.timeTrajectory_, controllerDataPtrArray);
      primalSolution.controllerPtr_.reset(
          new FeedforwardController(std::move(controller)));
      break;
    }
    case ocs2_msgs::msg::MpcFlattenedController::CONTROLLER_LINEAR: {
      auto controller = LinearController::unFlatten(
          stateDim, inputDim, primalSolution.timeTrajectory_,
          controllerDataPtrArray);
      primalSolution.controllerPtr_.reset(
          new LinearController(std::move(controller)));
      break;
    }
    default:
      throw std::runtime_error(
          "[MRT_ROS_Interface::readPolicyMsg] Unknown controllerType!");
  }
}
```

`MpcFlattenedController`消息里包含了本次`MPC`启动时的命令`CommandData`,这里包含了初始观测信息，以及期望跟踪轨迹。

```CPP
/**
 * This class contains the policy requirements and desired set-point.
 */
struct CommandData {
  SystemObservation mpcInitObservation_;
  TargetTrajectories mpcTargetTrajectories_;
};
```

还包含了原问题的解`primalSolution`,包含了`MPC`解时间序列，状态序列，期望输入序列，以及控制器控制序列`u`等

```CPP

/**
 * This class contains the primal problem's solution.
 */
struct PrimalSolution {
  /** Constructor */
  PrimalSolution() = default;

  /** Destructor */
  ~PrimalSolution() = default;

  /** Copy constructor */
  PrimalSolution(const PrimalSolution& other)
      : timeTrajectory_(other.timeTrajectory_),
        stateTrajectory_(other.stateTrajectory_),
        inputTrajectory_(other.inputTrajectory_),
        postEventIndices_(other.postEventIndices_),
        modeSchedule_(other.modeSchedule_),
        controllerPtr_(other.controllerPtr_ ? other.controllerPtr_->clone() : nullptr) {}

  /** Copy Assignment */
  PrimalSolution& operator=(const PrimalSolution& other) {
    timeTrajectory_ = other.timeTrajectory_;
    stateTrajectory_ = other.stateTrajectory_;
    inputTrajectory_ = other.inputTrajectory_;
    postEventIndices_ = other.postEventIndices_;
    modeSchedule_ = other.modeSchedule_;
    if (other.controllerPtr_) {
      controllerPtr_.reset(other.controllerPtr_->clone());
    } else {
      controllerPtr_.reset();
    }
    return *this;
  }

  /** Move constructor */
  PrimalSolution(PrimalSolution&& other) noexcept = default;

  /** Move Assignment */
  PrimalSolution& operator=(PrimalSolution&& other) noexcept = default;

  /** Swap */
  void swap(PrimalSolution& other) {
    timeTrajectory_.swap(other.timeTrajectory_);
    stateTrajectory_.swap(other.stateTrajectory_);
    inputTrajectory_.swap(other.inputTrajectory_);
    postEventIndices_.swap(other.postEventIndices_);
    ::ocs2::swap(modeSchedule_, other.modeSchedule_);
    controllerPtr_.swap(other.controllerPtr_);
  }

  void clear() {
    timeTrajectory_.clear();
    stateTrajectory_.clear();
    inputTrajectory_.clear();
    postEventIndices_.clear();
    modeSchedule_.clear();
    if (controllerPtr_ != nullptr) {
      controllerPtr_->clear();
    }
  }

  scalar_array_t timeTrajectory_;
  vector_array_t stateTrajectory_;
  vector_array_t inputTrajectory_;
  size_array_t postEventIndices_;
  ModeSchedule modeSchedule_;
  std::unique_ptr<ControllerBase> controllerPtr_;
};
```

还包含了`PerformanceIndex`这是`MPC`求解的性能指标

```CPP

/**
 * Defines the performance indices for a rollout
 */
struct PerformanceIndex {
  /** The merit function of a rollout. */
  scalar_t merit = 0.0;

  /** The total cost of a rollout. */
  scalar_t cost = 0.0;

  /** Sum of Squared Error (SSE) of the dual feasibilities:
   * - Final: squared norm of violation in the dual feasibilities
   * - PreJumps: sum of squared norm of violation in the dual feasibilities
   * - Intermediates: sum of squared norm of violation in the dual feasibilities
   */
  scalar_t dualFeasibilitiesSSE = 0.0;

  /** Sum of Squared Error (SSE) of system dynamics violation */
  scalar_t dynamicsViolationSSE = 0.0;

  /** Sum of Squared Error (SSE) of equality constraints:
   * - Final: squared norm of violation in state equality constraints
   * - PreJumps: sum of squared norm of violation in state equality constraints
   * - Intermediates: Integral of squared norm violation in state/state-input equality constraints
   */
  scalar_t equalityConstraintsSSE = 0.0;

  /** Sum of Squared Error (SSE) of inequality constraints:
   * - Final: squared norm of violation in state inequality constraints
   * - PreJumps: sum of squared norm of violation in state inequality constraints
   * - Intermediates: Integral of squared norm violation in state/state-input inequality constraints
   */
  scalar_t inequalityConstraintsSSE = 0.0;

  /** Sum of equality Lagrangians:
   * - Final: penalty for violation in state equality constraints
   * - PreJumps: penalty for violation in state equality constraints
   * - Intermediates: penalty for violation in state/state-input equality constraints
   */
  scalar_t equalityLagrangian = 0.0;

  /** Sum of inequality Lagrangians:
   * - Final: penalty for violation in state inequality constraints
   * - PreJumps: penalty for violation in state inequality constraints
   * - Intermediates: penalty for violation in state/state-input inequality constraints
   */
  scalar_t inequalityLagrangian = 0.0;

  /** Add performance indices. */
  PerformanceIndex& operator+=(const PerformanceIndex& rhs);

  /** Multiply by a scalar. */
  PerformanceIndex& operator*=(const scalar_t c);

  /** Returns true if *this is approximately equal to other, within the precision determined by prec. */
  bool isApprox(const PerformanceIndex& other, const scalar_t prec = 1e-8) const;
};
```

将`ROS`消息转化为`primalSolution`,`CommandData`,`PerformanceIndex`后，将其保存在基类的`Buffer`中，便于异步取出.

### 基类MRT_BASE

这个基类真正存储了当前传递进来的`MPC`解，可以获取当前最新的解，使用系统模型推导下一时刻状态。

#### 使用当前输入推导状态

```CPP
void MRT_BASE::initRollout(const RolloutBase* rolloutPtr) {
  rolloutPtr_.reset(rolloutPtr->clone());
}
```

```CPP
void MRT_BASE::rolloutPolicy(scalar_t currentTime, const vector_t& currentState, const scalar_t& timeStep, vector_t& mpcState,
                             vector_t& mpcInput, size_t& mode) {
  if (rolloutPtr_ == nullptr) {
    throw std::runtime_error("[MRT_BASE::rolloutPolicy] rollout class is not set! Use initRollout() to initialize it!");
  }

  if (activePrimalSolutionPtr_ == nullptr) {
    throw std::runtime_error("[MRT_BASE::rolloutPolicy] updatePolicy() should be called first!");
  }

  if (currentTime > activePrimalSolutionPtr_->timeTrajectory_.back()) {
    std::cerr << "The requested currentTime is greater than the received plan: " << std::to_string(currentTime) << ">"
              << std::to_string(activePrimalSolutionPtr_->timeTrajectory_.back()) << "\n";
  }

  // perform a rollout
  scalar_array_t timeTrajectory;
  size_array_t postEventIndicesStock;
  vector_array_t stateTrajectory, inputTrajectory;
  const scalar_t finalTime = currentTime + timeStep;
  rolloutPtr_->run(currentTime, currentState, finalTime, activePrimalSolutionPtr_->controllerPtr_.get(),
                   activePrimalSolutionPtr_->modeSchedule_, timeTrajectory, postEventIndicesStock, stateTrajectory, inputTrajectory);

  mpcState = stateTrajectory.back();
  mpcInput = inputTrajectory.back();

  mode = activePrimalSolutionPtr_->modeSchedule_.modeAtTime(finalTime);
}
```

使用当前输入推导系统之后的状态,也就是将系统动力学与给定的控制器进行前向积分，来推导系统指定时间长的状态.

## MRT_ROS_Dummy_Loop

`MRT_ROS_Dummy_Loop`使用数值积分虚拟出系统状态测量，可以用来测试`MPC`,`MRT`与`ROS2`的通信.

### 关键成员变量

```CPP
MRT_ROS_Interface& mrt_;
std::vector<std::shared_ptr<DummyObserver>> observers_;

scalar_t mrtDesiredFrequency_;
scalar_t mpcDesiredFrequency_;
```

`mrt_`保存了要操作的`MRT_ROS_Interface`

`observers_`给了一个探测当前仿真数值等内容的接口.

`mrtDesiredFrequency_`表示期望虚拟状态测量传递给`MRT`的频率.

`mpcDesiredFrequency_`表示期望`MPC`运行的速率，其实就是期望`mrt_.setCurrentObservation`运行的频率。并不是实际时间，只是仿真出来的时间.

### 运行run

```CPP
void MRT_ROS_Dummy_Loop::run(const SystemObservation& initObservation,
                             const TargetTrajectories& initTargetTrajectories) {
  RCLCPP_INFO_STREAM(LOGGER, "Waiting for the initial policy ...");

  // Reset MPC node
  mrt_.resetMpcNode(initTargetTrajectories);

  // Wait for the initial policy
  while (!mrt_.initialPolicyReceived() && rclcpp::ok()) {
    mrt_.spinMRT();
    mrt_.setCurrentObservation(initObservation);
    rclcpp::Rate(mrtDesiredFrequency_).sleep();
  }
  RCLCPP_INFO_STREAM(LOGGER, "Initial policy has been received.");

  // Pick simulation loop mode
  if (mpcDesiredFrequency_ > 0.0) {
    synchronizedDummyLoop(initObservation, initTargetTrajectories);
  } else {
    realtimeDummyLoop(initObservation, initTargetTrajectories);
  }
}
```

通过操控`mrt_`来控制`MPC`.

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void MRT_ROS_Dummy_Loop::synchronizedDummyLoop(
    const SystemObservation& initObservation,
    const TargetTrajectories& initTargetTrajectories) {
  // Determine the ratio between MPC updates and simulation steps.
  const auto mpcUpdateRatio =
      std::max(static_cast<size_t>(mrtDesiredFrequency_ / mpcDesiredFrequency_),
               size_t(1));

  // Loop variables
  size_t loopCounter = 0;
  SystemObservation currentObservation = initObservation;

  // Helper function to check if policy is updated and starts at the given time.
  // Due to ROS message conversion delay and very fast MPC loop, we might get an
  // old policy instead of the latest one.
  const auto policyUpdatedForTime = [this](scalar_t time) {
    constexpr scalar_t tol =
        0.1;  // policy must start within this fraction of dt
    return mrt_.updatePolicy() &&
           std::abs(mrt_.getPolicy().timeTrajectory_.front() - time) <
               (tol / mpcDesiredFrequency_);
  };

  rclcpp::Rate simRate(mrtDesiredFrequency_);
  while (rclcpp::ok()) {
    std::cout << "### Current time " << currentObservation.time << "\n";

    // Trigger MRT callbacks
    mrt_.spinMRT();

    // Update the MPC policy if it is time to do so
    if (loopCounter % mpcUpdateRatio == 0) {
      // Wait for the policy to be updated
      while (!policyUpdatedForTime(currentObservation.time) && rclcpp::ok()) {
        mrt_.spinMRT();
      }
      std::cout << "<<< New MPC policy starting at "
                << mrt_.getPolicy().timeTrajectory_.front() << "\n";
    }

    // Forward simulation
    currentObservation = forwardSimulation(currentObservation);

    // User-defined modifications before publishing
    modifyObservation(currentObservation);

    // Publish observation if at the next step we want a new policy
    if ((loopCounter + 1) % mpcUpdateRatio == 0) {
      mrt_.setCurrentObservation(currentObservation);
      std::cout << ">>> Observation is published at " << currentObservation.time
                << "\n";
    }

    // Update observers
    for (auto& observer : observers_) {
      observer->update(currentObservation, mrt_.getPolicy(), mrt_.getCommand());
    }

    ++loopCounter;
    mrt_.spinMRT();
    simRate.sleep();
  }
}
```

在观测器`observer->update`可以获取当其仿真结果，`MPC`计算结果.

```CPP
class CartpoleDummyVisualization : public DummyObserver {
 public:
  explicit CartpoleDummyVisualization(const rclcpp::Node::SharedPtr& node);

  ~CartpoleDummyVisualization() override = default;

  void update(const SystemObservation& observation,
              const PrimalSolution& policy,
              const CommandData& command) override;

 private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr jointPublisher_;
};
CartpoleDummyVisualization::CartpoleDummyVisualization(
    const rclcpp::Node::SharedPtr& node)
    : node_(node),
      jointPublisher_(node_->create_publisher<sensor_msgs::msg::JointState>(
          "joint_states", 1)) {}
void CartpoleDummyVisualization::update(const SystemObservation& observation,
                                        const PrimalSolution& policy,
                                        const CommandData& command) {
  sensor_msgs::msg::JointState joint_state;
  joint_state.header.stamp = node_->get_clock()->now();
  joint_state.name.resize(2);
  joint_state.position.resize(2);
  joint_state.name[0] = "slider_to_cart";
  joint_state.name[1] = "cart_to_pole";
  joint_state.position[0] = observation.state(1);
  joint_state.position[1] = observation.state(0);
  jointPublisher_->publish(joint_state);
}
```

```CPP
SystemObservation MRT_ROS_Dummy_Loop::forwardSimulation(
    const SystemObservation& currentObservation) {
  const scalar_t dt = 1.0 / mrtDesiredFrequency_;

  SystemObservation nextObservation;
  nextObservation.time = currentObservation.time + dt;
  if (mrt_.isRolloutSet()) {  // If available, use the provided rollout as to
                              // integrate the dynamics.
    mrt_.rolloutPolicy(currentObservation.time, currentObservation.state, dt,
                       nextObservation.state, nextObservation.input,
                       nextObservation.mode);
  } else {  // Otherwise, we fake integration by interpolating the current MPC
            // policy at t+dt
    mrt_.evaluatePolicy(currentObservation.time + dt, currentObservation.state,
                        nextObservation.state, nextObservation.input,
                        nextObservation.mode);
  }

  return nextObservation;
}
```

进行状态仿真.使用仿真结果更新当前观测.
