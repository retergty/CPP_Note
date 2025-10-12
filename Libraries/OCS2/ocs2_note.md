# OCS2

参考文档

* [官方文档](https://leggedrobotics.github.io/ocs2/overview.html)

`OCS2`是一个专为切换系统最优控制(`OCS2`)定制的`C++`工具箱。该工具箱提供了以下算法的有效实现

* `SLQ`,连续时间限制DDP。
* `iLQR`离散时间限制DDP。
* `SQP`基于`HPIPM`的多发射算法。
* `SLP`基于`PIPG`的序贯线性规划。
* `IPM`基于非线性内点法的多发射算法。

## 最优控制模型

`OCS2`求解切换系统的最优控制问题，切换系统由有限数量的动态子系统组成，这些子系统受到离散事件的影响，这些事件会导致这些子系统之间的转换。

$$
\begin{split}
    \begin{cases}
    \underset{\mathbf u(.)}{\min} \ \ \sum_i \phi_i(\mathbf x(t_{i+1})) + \displaystyle \int_{t_i}^{t_{i+1}} l_i(\mathbf x(t), \mathbf u(t), t) \, dt \\
    \text{s.t.} \ \ \mathbf x(t_0) = \mathbf x_0 \,\hspace{12em} \text{initial state} \\
    \ \ \ \ \ \dot{\mathbf x}(t) = \mathbf f_i(\mathbf x(t), \mathbf u(t), t) \hspace{8em} \text{system flow map} \\
    \ \ \ \ \ \mathbf x(t_{i+1}^+) = \mathbf j(\mathbf x(t_{i+1})) \hspace{9em} \text{system jump map} \\
    \ \ \ \ \ {\mathbf g_1}_i(\mathbf x(t), \mathbf u(t), t) = \mathbf{0} \hspace{8.5em} \text{state-input equality constraints} \\
    \ \ \ \ \ {\mathbf g_2}_i(\mathbf x(t), t) = \mathbf{0} \, \hspace{10.5em}  \text{state-only equality constraints} \\
    \ \ \ \ \ \mathbf h_i(\mathbf x(t), \mathbf u(t), t) \geq \mathbf{0} \hspace{9em}  \text{inequality constraints} \\
    \ \ \ \ \ \text{for  } t_i < t < t_{i+1} \text{  and  } i \in \{0, 1, \cdots, I-1 \}
    \end{cases}\end{split}
$$

其中$t_i$是切换时间，$t_I$是最终时间。相当于一系列的子问题通过`system jump map`串联了起来。

对于非切换系统，问题定义如下

$$
\begin{split}
    \begin{cases}
    \underset{\mathbf u(.)}{\min} \ \ \phi(\mathbf x(t_I)) + \displaystyle \int_{t_0}^{t_I} l(\mathbf x(t), \mathbf u(t), t) \, dt \\
    \text{s.t.} \ \ \mathbf x(t_0) = \mathbf x_0 \,\hspace{11.5em} \text{initial state} \\
    \ \ \ \ \ \dot{\mathbf x}(t) = \mathbf f(\mathbf x(t), \mathbf u(t), t) \hspace{7.5em} \text{system flow map} \\
    \ \ \ \ \ \mathbf g_1(\mathbf x(t), \mathbf u(t), t) = \mathbf{0} \hspace{8.5em} \text{state-input equality constraints} \\
    \ \ \ \ \ \mathbf g_2(\mathbf x(t), t) = \mathbf{0}  \hspace{10.5em}  \text{state-only equality constraints}  \\
    \ \ \ \ \ \mathbf h(\mathbf x(t), \mathbf u(t), t) \geq \mathbf{0} \hspace{8.5em}  \text{inequality constraints}
    \end{cases}\end{split}
$$

## 代价函数Cost

代价函数`Cost`分为三种，中间代价`intermediate`,切换前代价`prejump`,最终代价`final`.

`ocs2`假定中间代价`intermediate`函数的`Hession`矩阵和在任何时候都是正定的.

## 约束Constraints

约束`Constraints`也分为三种，中间约束`intermediate`,切换前约束`prejump`,最终约束`final`.

中间约束可以是时间，状态，输入的函数，但最终约束只能是时间与状态的函数.约束条件应该从`StateConstraint`或`StateInputConstraint`类继承。派生类应该根据约束的次数定义约束值及其线性或二次近似值。

为了处理`OCS2`中的约束，可以使用硬约束或软约束方法.

软约束由`OptimalControlProblem`单独收集,软约束处理基于惩罚方法，其中约束被用户定义的惩罚函数所包裹（有关这些惩罚函数的列表，请参阅`ocs2_core/soft_constraint/penalties`）。要从约束项创建软约束，可以使用`StateSoftConstraint`和 `StateInputSoftConstraint`类。可以为每个软约束设置自己的惩罚函数,十分灵活.

硬约束（称为约束）根据其类型通过不同的技术以更高的精度处理，状态，输入等式约束通过投影方法处理。状态等式与所有不等式通过松弛障碍法或增广拉格朗日法(禁用).

由于状态输入等式约束是通过投影方法处理的，因此`OCS2`假设约束相对于输入的雅可比矩阵是满行秩.如果无法保证这个条件，则应该使用软约束技术。

## 代码分析

### 最优控制问题结构体OptimalControlProblem

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

这个结构体定义MPC问题中的主要组成部分，包括系统动态，成本，约束三个部分。

总的来说，三个部分都包含三种时间类型定义，（1）在中间时间间隔内，（2）在切换前时间，以及（3）在优化范围的最后时间。

用户可以传递一系列的成本或约束，并需要每一个命名，还可以选择当前是否活跃(active)。

## ReferenceManagerInterface

`ReferenceManagerInterface`创建一个通用接口，用于线程安全向`MPC`内部传递目标轨迹和模式切换调度（仅在切换系统中使用）。`OCS2`的每个求解器在开始 `MPC`的新迭代之前都会调用`ReferenceManagerInterface`的preSolverRun()方法.`ReferenceManager`有两个装饰器类：`ReferenceManagerRos`，它将 `ROS`通信添加到`ReferenceManager`和`LoopshapingReferenceManager`，它将其扩展到循环形状的`OptimalControlProblem`。

`ReferenceManagerInterface`在`MPC`主循环中运行，所以需要防止消耗事件的操作.
