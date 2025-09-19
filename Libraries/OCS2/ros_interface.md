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
