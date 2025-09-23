# 定义MPC问题

`OCS2`通过一个结构体定义`MPC`问题，通过一个特定的文件定义`MPC`求解参数.

```CPP
```CPP
/** Optimal Control Problem definition */
struct OptimalControlProblem {
  /* Cost */
  /** Intermediate cost */
  std::unique_ptr<StateInputCostCollection> costPtr;
  /** Intermediate state-only cost */
  std::unique_ptr<StateCostCollection> stateCostPtr;
  /** Pre-jump cost */
  std::unique_ptr<StateCostCollection> preJumpCostPtr;
  /** Final cost */
  std::unique_ptr<StateCostCollection> finalCostPtr;

  /* Soft constraints */
  /** Intermediate soft constraint penalty */
  std::unique_ptr<StateInputCostCollection> softConstraintPtr;
  /** Intermediate state-only soft constraint penalty */
  std::unique_ptr<StateCostCollection> stateSoftConstraintPtr;
  /** Pre-jump soft constraint penalty */
  std::unique_ptr<StateCostCollection> preJumpSoftConstraintPtr;
  /** Final soft constraint penalty */
  std::unique_ptr<StateCostCollection> finalSoftConstraintPtr;

  /* Constraints */
  /** Intermediate equality constraints, full row rank w.r.t. inputs */
  std::unique_ptr<StateInputConstraintCollection> equalityConstraintPtr;
  /** Intermediate state-only equality constraints */
  std::unique_ptr<StateConstraintCollection> stateEqualityConstraintPtr;
  /** Intermediate inequality constraints */
  std::unique_ptr<StateInputConstraintCollection> inequalityConstraintPtr;
  /** pre-jump constraints */
  std::unique_ptr<StateConstraintCollection> preJumpEqualityConstraintPtr;
  /** final constraints */
  std::unique_ptr<StateConstraintCollection> finalEqualityConstraintPtr;

  /* Dynamics */
  /** System dynamics pointer */
  std::unique_ptr<SystemDynamicsBase> dynamicsPtr;

  /* Misc. */
  /** The pre-computation module */
  std::unique_ptr<PreComputation> preComputationPtr;

  ...
}
```

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

## 非切换问题

非切换系统问题指的是没有多种状态切换的系统，这种系统较切换系统较为简单，本节以多种实际机器人模型为例子.

