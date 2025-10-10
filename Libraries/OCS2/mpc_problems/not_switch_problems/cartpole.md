# cartpole

## CartPoleInterface类

```CPP
class CartPoleInterface final : public RobotInterface {
 public:
  /**
   * Constructor
   *
   * @note Creates directory for generated library into if it does not exist.
   * @throw Invalid argument error if input task file does not exist.
   *
   * @param [in] taskFile: The absolute path to the configuration file for the MPC.
   * @param [in] libraryFolder: The absolute path to the directory to generate CppAD library into.
   * @param [in] verbose: Flag to determine to print out the loaded settings and status of complied libraries.
   */
  CartPoleInterface(const std::string& taskFile, const std::string& libraryFolder, bool verbose);

  /**
   * Destructor
   */
  ~CartPoleInterface() override = default;

  const vector_t& getInitialState() { return initialState_; }

  const vector_t& getInitialTarget() { return xFinal_; }

  ddp::Settings& ddpSettings() { return ddpSettings_; }

  mpc::Settings& mpcSettings() { return mpcSettings_; }

  OptimalControlProblem& optimalControlProblem() { return problem_; }
  const OptimalControlProblem& getOptimalControlProblem() const override { return problem_; }

  const RolloutBase& getRollout() const { return *rolloutPtr_; }

  const Initializer& getInitializer() const override { return *cartPoleInitializerPtr_; }

 private:
  ddp::Settings ddpSettings_;
  mpc::Settings mpcSettings_;

  OptimalControlProblem problem_;

  std::unique_ptr<RolloutBase> rolloutPtr_;
  std::unique_ptr<Initializer> cartPoleInitializerPtr_;

  vector_t initialState_{STATE_DIM};
  vector_t xFinal_{STATE_DIM};
};
```

`CartPoleInterface`类定义了倒立摆动力学，读取文件等.

### 成员变量

```CPP
ddp::Settings ddpSettings_;
mpc::Settings mpcSettings_;
```

是`mpc`与求解器`ddp`的设置

```CPP
OptimalControlProblem problem_;
```

倒立摆的最优控制总问题

```CPP
std::unique_ptr<RolloutBase> rolloutPtr_;
```

状态推导器

```CPP
std::unique_ptr<Initializer> cartPoleInitializerPtr_;
```

初始化器

### 构造函数

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
CartPoleInterface::CartPoleInterface(const std::string& taskFile, const std::string& libraryFolder, bool verbose) {
  // check that task file exists
  boost::filesystem::path taskFilePath(taskFile);
  if (boost::filesystem::exists(taskFilePath)) {
    std::cerr << "[CartPoleInterface] Loading task file: " << taskFilePath << "\n";
  } else {
    throw std::invalid_argument("[CartPoleInterface] Task file not found: " + taskFilePath.string());
  }
  // create library folder if it does not exist
  boost::filesystem::path libraryFolderPath(libraryFolder);
  boost::filesystem::create_directories(libraryFolderPath);
  std::cerr << "[CartPoleInterface] Generated library path: " << libraryFolderPath << "\n";

  // Default initial condition
  loadData::loadEigenMatrix(taskFile, "initialState", initialState_);
  loadData::loadEigenMatrix(taskFile, "x_final", xFinal_);
  if (verbose) {
    std::cerr << "x_init:   " << initialState_.transpose() << "\n";
    std::cerr << "x_final:  " << xFinal_.transpose() << "\n";
  }

  // DDP-MPC settings
  ddpSettings_ = ddp::loadSettings(taskFile, "ddp", verbose);
  mpcSettings_ = mpc::loadSettings(taskFile, "mpc", verbose);

  /*
   * Optimal control problem
   */
  // Cost
  matrix_t Q(STATE_DIM, STATE_DIM);
  matrix_t R(INPUT_DIM, INPUT_DIM);
  matrix_t Qf(STATE_DIM, STATE_DIM);
  loadData::loadEigenMatrix(taskFile, "Q", Q);
  loadData::loadEigenMatrix(taskFile, "R", R);
  loadData::loadEigenMatrix(taskFile, "Q_final", Qf);
  if (verbose) {
    std::cerr << "Q:  \n" << Q << "\n";
    std::cerr << "R:  \n" << R << "\n";
    std::cerr << "Q_final:\n" << Qf << "\n";
  }

  problem_.costPtr->add("cost", std::make_unique<QuadraticStateInputCost>(Q, R));
  problem_.finalCostPtr->add("finalCost", std::make_unique<QuadraticStateCost>(Qf));

  // Dynamics
  CartPoleParameters cartPoleParameters;
  cartPoleParameters.loadSettings(taskFile, "cartpole_parameters", verbose);
  problem_.dynamicsPtr.reset(new CartPoleSytemDynamics(cartPoleParameters, libraryFolder, verbose));

  // Rollout
  auto rolloutSettings = rollout::loadSettings(taskFile, "rollout", verbose);
  rolloutPtr_.reset(new TimeTriggeredRollout(*problem_.dynamicsPtr, rolloutSettings));

  // Constraints
  auto getPenalty = [&]() {
    // one can use either augmented::SlacknessSquaredHingePenalty or augmented::ModifiedRelaxedBarrierPenalty
    using penalty_type = augmented::SlacknessSquaredHingePenalty;
    penalty_type::Config boundsConfig;
    loadData::loadPenaltyConfig(taskFile, "bounds_penalty_config", boundsConfig, verbose);
    return penalty_type::create(boundsConfig);
  };
  auto getConstraint = [&]() {
    constexpr size_t numIneqConstraint = 2;
    const vector_t e = (vector_t(numIneqConstraint) << cartPoleParameters.maxInput_, cartPoleParameters.maxInput_).finished();
    const vector_t D = (vector_t(numIneqConstraint) << 1.0, -1.0).finished();
    const matrix_t C = matrix_t::Zero(numIneqConstraint, STATE_DIM);
    return std::make_unique<LinearStateInputConstraint>(e, C, D);
  };
  problem_.inequalityLagrangianPtr->add("InputLimits", create(getConstraint(), getPenalty()));

  // Initialization
  cartPoleInitializerPtr_.reset(new DefaultInitializer(INPUT_DIM));
}
```

读取`taskfile`文件，这个文件定义了初始状态，最终状态等，权重矩阵，`mpc`求解参数设置，`ddp`求解参数设置.

`Q`是状态权重矩阵，`R`是控制权重矩阵，`Q_final`是优化末端状态权重矩阵.

`problem_`是总的最优控制问题，通过`add`即可把代价函数加入。

### task.info

```txt
; cartpole parameters
cartpole_parameters
{
  cartMass     2.0
  poleMass     0.2
  poleLength   1.0
  maxInput     5.0
  gravity      9.81
}

; DDP settings
ddp
{
  algorithm                      SLQ

  nThreads                       2

  maxNumIterations               1
  minRelCost                     0.1
  constraintTolerance            1e-3

  displayInfo                    false
  displayShortSummary            false
  checkNumericalStability        false

  AbsTolODE                      1e-9
  RelTolODE                      1e-6
  maxNumStepsPerSecond           100000
  timeStep                       1e-2
  backwardPassIntegratorType     ODE45

  inequalityConstraintMu         100.0
  inequalityConstraintDelta      1.1

  preComputeRiccatiTerms         true

  useFeedbackPolicy              false

  strategy                       LINE_SEARCH
  lineSearch
  {
    minStepLength                1e-3
    maxStepLength                1.0
    hessianCorrectionStrategy    EIGENVALUE_MODIFICATION
    hessianCorrectionMultiple    1e-6
  }
}

; Rollout settings
rollout
{
  AbsTolODE                    1e-9
  RelTolODE                    1e-6
  timeStep                     1e-2
  maxNumStepsPerSecond         100000
  checkNumericalStability      false
  integratorType               ODE45
}

; MPC settings
mpc
{
  timeHorizon                 5.0   ; [s]
  solutionTimeWindow          -1    ; maximum [s]
  coldStart                   false

  debugPrint                  false

  mpcDesiredFrequency         100   ; [Hz]
  mrtDesiredFrequency         400   ; [Hz]
}

bounds_penalty_config
{
  scale                       0.1
  stepSize                    1.0
}

; initial state
initialState
{
  (0,0) 3.14   ; theta
  (1,0) 0.0    ; x
  (2,0) 0.0    ; theta_dot
  (3,0) 0.0    ; x_dot
}

; state weight matrix
Q
{
  (0,0)  0.0   ; theta
  (1,1)  0.0   ; x
  (2,2)  0.0   ; theta_dot
  (3,3)  0.0   ; x_dot
}


; control weight matrix
R
{
  (0,0)  0.1
}


; final state weight matrix
Q_final
{
  (0,0)  5.0  ; theta
  (1,1)  1.0  ; x
  (2,2)  1.0  ; theta_dot
  (3,3)  1.0  ; x_dot
}

; final goal
x_final
{
  (0,0)  0.0  ; theta
  (1,0)  0.0  ; x
  (2,0)  0.0  ; theta_dot
  (3,0)  0.0  ; x_dot
}
```
