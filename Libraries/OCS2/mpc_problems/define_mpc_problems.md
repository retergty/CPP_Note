# 定义MPC问题

`OCS2`通过一个结构体定义`MPC`问题，通过一个特定的文件定义`MPC`求解参数.

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
  /** Pre-jump equality constraints */
  std::unique_ptr<StateConstraintCollection> preJumpEqualityConstraintPtr;
  /** Final equality constraints */
  std::unique_ptr<StateConstraintCollection> finalEqualityConstraintPtr;

  /* Inequality Constraints */
  /** Intermediate inequality constraints */
  std::unique_ptr<StateInputConstraintCollection> inequalityConstraintPtr;
  /** Intermediate state-only inequality constraints */
  std::unique_ptr<StateConstraintCollection> stateInequalityConstraintPtr;
  /** Pre-jump inequality constraints */
  std::unique_ptr<StateConstraintCollection> preJumpInequalityConstraintPtr;
  /** Final inequality constraints */
  std::unique_ptr<StateConstraintCollection> finalInequalityConstraintPtr;

  /* Lagrangians */
  /** Lagrangian for intermediate equality constraints */
  std::unique_ptr<StateInputAugmentedLagrangianCollection> equalityLagrangianPtr;
  /** Lagrangian for intermediate state-only equality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> stateEqualityLagrangianPtr;
  /** Lagrangian for intermediate inequality constraints */
  std::unique_ptr<StateInputAugmentedLagrangianCollection> inequalityLagrangianPtr;
  /** Lagrangian for intermediate state-only inequality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> stateInequalityLagrangianPtr;
  /** Lagrangian for pre-jump equality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> preJumpEqualityLagrangianPtr;
  /** Lagrangian for pre-jump inequality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> preJumpInequalityLagrangianPtr;
  /** Lagrangian for final equality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> finalEqualityLagrangianPtr;
  /** Lagrangian for final inequality constraints */
  std::unique_ptr<StateAugmentedLagrangianCollection> finalInequalityLagrangianPtr;

  /* Dynamics */
  /** System dynamics pointer */
  std::unique_ptr<SystemDynamicsBase> dynamicsPtr;

  /* Misc. */
  /** The pre-computation module */
  std::unique_ptr<PreComputation> preComputationPtr;

  /** The cost desired trajectories (will be substitute by ReferenceManager) */
  const TargetTrajectories* targetTrajectoriesPtr;

  /** Default constructor */
  OptimalControlProblem();

  /** Default destructor */
  ~OptimalControlProblem() = default;

  /** Copy constructor */
  OptimalControlProblem(const OptimalControlProblem& other);

  /** Copy assignment */
  OptimalControlProblem& operator=(const OptimalControlProblem& rhs);

  /** Move constructor */
  OptimalControlProblem(OptimalControlProblem&& other) noexcept = default;

  /** Move assignment */
  OptimalControlProblem& operator=(OptimalControlProblem&& rhs) noexcept = default;

  /** Swap */
  void swap(OptimalControlProblem& other) noexcept;
};
```

## Collection类

```CPP

/**
 * Implements the common add/get interface for cost and constraint collections.
 *
 * @tparam T : Type of the terms in the collection.
 */
template <typename T>
class Collection {
 public:
  Collection() = default;
  virtual ~Collection() = default;
  virtual Collection* clone() const { return new Collection(*this); }

  /** Checks if the collection has no elements */
  bool empty() const { return terms_.empty(); }

  /** Erases all elements from the Collection. */
  void clear();

  /**
   * Adds a term to the collection, and transfer ownership to the collection
   * The provided name must be unique and is later used to access the cost term.
   * @param name: Name stored along with the term.
   * @param term: Term to be added.
   */
  void add(std::string name, std::unique_ptr<T> term);

  /**
   * Erases a term from the collection.
   *
   * @param name: Name of the term.
   * @return True if the term was in the Collection and false if the term was not found in the Collection.
   */
  bool erase(const std::string& name) { return (extract(name) != nullptr); }

  /**
   * Removes a term from the Collection and returns it as a unique_ptr.
   *
   * @param name: Name of the term.
   * @return A unique pointer to the extracted term. If the term was not found it returns nullptr.
   */
  std::unique_ptr<T> extract(const std::string& name);

  /**
   * Use to modify a term.
   * @tparam Derived: derived class of base type T to cast to. Casts to the base class by default
   * @param name: Name of the term to modify
   * @return A reference to the underlying term
   */
  template <typename Derived = T>
  Derived& get(const std::string& name);

  /**
   * Finds the index of the term in the stored map.
   *
   * @param [in] name: Name of the term.
   * @param [out] index : Term index.
   * @return True if the name found in the collection.
   */
  bool getTermIndex(const std::string& name, size_t& index) const;

 protected:
  /** Copy constructor */
  Collection(const Collection& other);

  //! Contains all terms in the order they were added
  std::vector<std::unique_ptr<T>> terms_;

 private:
  //! Lookup from cost term name to index in the cost term vector
  std::unordered_map<std::string, size_t> termNameMap_;
};
```

`Collection`类是一个容器，保存了一系列的`T`,可以通过名称查找，添加，删除.

### StateInputCostCollection

以`StateInputCostCollection`为例

```CPP
/**
 * State Input Cost function combining a collection of cost terms.
 *
 * This class collects a variable number of cost terms and provides methods to get the
 * summed cost values and quadratic approximations. Each cost term can be accessed through its
 * string name and can be activated or deactivated.
 */
class StateInputCostCollection : public Collection<StateInputCost> {
 public:
  StateInputCostCollection() = default;
  ~StateInputCostCollection() override = default;
  StateInputCostCollection* clone() const override;

  /** Get state-input cost value */
  virtual scalar_t getValue(scalar_t time, const vector_t& state, const vector_t& input, const TargetTrajectories& targetTrajectories,
                            const PreComputation& preComp) const;

  /** Get state-input cost quadratic approximation */
  virtual ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                         const TargetTrajectories& targetTrajectories,
                                                                         const PreComputation& preComp) const;

 protected:
  /** Copy constructor */
  StateInputCostCollection(const StateInputCostCollection& other);
};
```

`StateInputCostCollection`是用于定义状态-输入代价函数集合的类。

`getValue`迭代所有已有的状态-输入代价函数，将值加起来.

`getQuadraticApproximation`迭代所有已有的状态-输入代价函数，将二次泰勒参数相加.

## Approximation类

这种类包含`VectorFunctionLinearApproximation`,`ScalarFunctionQuadraticApproximation`是函数的泰勒展开估计。

### ScalarFunctionQuadraticApproximation

```CPP
/**
 * Defines the quadratic approximation of a scalar function
 * f(x,u) = 1/2 dx' dfdxx dx + du' dfdux dx + 1/2 du' dfduu du + dfdx' dx + dfdu' du + f
 */
struct ScalarFunctionQuadraticApproximation {
  /** Second derivative w.r.t state */
  matrix_t dfdxx;
  /** Second derivative w.r.t input (lhs) and state (rhs) */
  matrix_t dfdux;
  /** Second derivative w.r.t input */
  matrix_t dfduu;
  /** First derivative w.r.t state */
  vector_t dfdx;
  /** First derivative w.r.t input */
  vector_t dfdu;
  /** Constant term */
  scalar_t f = 0.;

  /** Default constructor */
  ScalarFunctionQuadraticApproximation() = default;

  /** Construct and resize the members to given size. Pass nu = -1 for no inputs */
  explicit ScalarFunctionQuadraticApproximation(int nx, int nu = -1);

  /** Compound addition assignment operator */
  ScalarFunctionQuadraticApproximation& operator+=(const ScalarFunctionQuadraticApproximation& rhs);

  /** Compound scalar multiplication and assignment operator */
  ScalarFunctionQuadraticApproximation& operator*=(scalar_t scalar);

  /**
   * Resize the members to the given size
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   */
  ScalarFunctionQuadraticApproximation& resize(int nx, int nu = -1);

  /**
   * Resizes the members to the given size, and sets all coefficients to zero.
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   */
  ScalarFunctionQuadraticApproximation& setZero(int nx, int nu = -1);

  /**
   * Factory function with zero initialization
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   * @return Zero initialized object of given size.
   */
  static ScalarFunctionQuadraticApproximation Zero(int nx, int nu = -1);
};
```

这是一个结构体，保存了标量函数的二阶泰勒展开.

$$
f(x,u) = \frac{1}{2}\textbf{x}^T\nabla_{\textbf{xx}}f(\bar{\textbf{x}},\bar{\textbf{u}})\textbf{x}
+ \frac{1}{2}\textbf{u}^T\nabla_{\textbf{uu}}f(\bar{\textbf{x}},\bar{\textbf{u}})\textbf{u}
+ \textbf{u}^T\nabla_{\textbf{xu}}f(\bar{\textbf{x}},\bar{\textbf{u}})\textbf{x}
+ \nabla_\textbf{x}f(\bar{\textbf{x}},\bar{\textbf{u}})^T\textbf{x}
+ \nabla_\textbf{u}f(\bar{\textbf{x}},\bar{\textbf{u}})^T\textbf{u}
+ f_0
$$

### VectorFunctionLinearApproximation

```CPP
/**
 * Defines the linear model of a vector-valued function
 * f(x,u) = dfdx dx + dfdu du + f
 */
struct VectorFunctionLinearApproximation {
  /** Derivative w.r.t state */
  matrix_t dfdx;
  /** Derivative w.r.t input */
  matrix_t dfdu;
  /** Constant term */
  vector_t f;

  /** Default constructor */
  VectorFunctionLinearApproximation() = default;

  /** Construct and resize the members to given size. (Pass nu = -1 for no inputs) */
  explicit VectorFunctionLinearApproximation(int nv, int nx, int nu = -1);

  /**
   * Resize the members to the given size
   * @param[in] nv Vector dimension
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   */
  VectorFunctionLinearApproximation& resize(int nv, int nx, int nu = -1);

  /**
   * Resizes the members to the given size, and sets all coefficients to zero.
   * @param[in] nv Vector dimension
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   */
  VectorFunctionLinearApproximation& setZero(int nv, int nx, int nu = -1);

  /**
   * Factory function with zero initialization
   * @param[in] nv Vector dimension
   * @param[in] nx State dimension
   * @param[in] nu Input dimension (Pass nu = -1 for no inputs)
   * @return Zero initialized object of given size.
   */
  static VectorFunctionLinearApproximation Zero(int nv, int nx, int nu = -1);
};
```

这是一个结构体，保存了向量函数的线性泰勒展开.

$$
f(x,u) = \nabla_\textbf{x}f(\bar{\textbf{x}},\bar{\textbf{u}}) \delta\textbf{x} + \nabla_\textbf{u}f(\bar{\textbf{x}},\bar{\textbf{u}})\delta\textbf{u} + f(\bar{\textbf{x}},\bar{\textbf{u}})
$$

## 声明系统动态SystemDynamics

### OdeBase类

```CPP

/**
 * The base class for autonomous system dynamics.
 */
class OdeBase {
 public:
  /** Default constructor */
  OdeBase() = default;

  /** Default destructor */
  virtual ~OdeBase() = default;

  /** Returns the number of function calls. */
  size_t getNumFunctionCalls() const { return numFunctionCalls_; }

  /** Resets the number of function calls to zero. */
  void resetNumFunctionCalls() { numFunctionCalls_ = 0; }

  /** Increments the number of function calls. */
  size_t incrementNumFunctionCalls() { return ++numFunctionCalls_; }

  /**
   * Computes the autonomous system dynamics.
   * @param [in] t: Current time.
   * @param [in] x: Current state.
   * @return Current state time derivative
   */
  virtual vector_t computeFlowMap(scalar_t t, const vector_t& x) = 0;

  /**
   * State map at the transition time
   *
   * @param [in] time: transition time
   * @param [in] state: transition state
   * @return mapped state after transition
   */
  virtual vector_t computeJumpMap(scalar_t time, const vector_t& state);

  /**
   * Interface method to the guard surfaces.
   *
   * @param [in] time: transition time
   * @param [in] state: transition state
   * @return An array of guard surfaces values
   */
  virtual vector_t computeGuardSurfaces(scalar_t time, const vector_t& state);

 protected:
  /** Copy constructor */
  OdeBase(const OdeBase& rhs) : numFunctionCalls_(0) {}

 private:
  size_t numFunctionCalls_ = 0;
};
```

自动系统动态基类，也就是没有输入，只凭借当前状态自动运行的系统.

```CPP
/**
 * Computes the autonomous system dynamics.
 * @param [in] t: Current time.
 * @param [in] x: Current state.
 * @return Current state time derivative
 */
virtual vector_t computeFlowMap(scalar_t t, const vector_t& x) = 0;
```

可见，这个系统动态函数只有当前时间$t$与当前状态$\textbf{x}$,返回$\frac{d\textbf{x}}{dt}$

### ControlledSystemBase类

```CPP
/**
 * The base class for non-autonomous system dynamics.
 */
class ControlledSystemBase : public OdeBase {
 public:
  /**
   * Constructor
   *
   * @param [in] preComputation: The (optional) pre-computation module, internally keeps a copy.
   *                             @see PreComputation class documentation.
   */
  explicit ControlledSystemBase(const PreComputation& preComputation = PreComputation());

  /** Default destructor */
  ~ControlledSystemBase() override = default;

  /** Clone */
  virtual ControlledSystemBase* clone() const = 0;

  /** Resets the internal classes. */
  virtual void reset() { controllerPtr_ = nullptr; }

  /**
   * Sets the control policy using the controller class.
   */
  void setController(ControllerBase* controllerPtr) { controllerPtr_ = controllerPtr; };

  /**
   * Returns the controller pointer.
   */
  ControllerBase* controllerPtr() const { return controllerPtr_; };

  /**
   * Computes the flow map of a system.
   *
   * @param [in] t: The current time.
   * @param [in] x: The current state.
   * @return The state time derivative.
   */
  vector_t computeFlowMap(scalar_t t, const vector_t& x) override final;

  /**
   * Computes the flow map of a system with exogenous input.
   *
   * @param [in] t: The current time.
   * @param [in] x: The current state.
   * @param [in] u: The current input.
   * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
   *                      @see PreComputation class documentation.
   * @return The state time derivative.
   */
  virtual vector_t computeFlowMap(scalar_t t, const vector_t& x, const vector_t& u, const PreComputation& preComp) = 0;

  /**
   * State map at the transition time
   *
   * @param [in] time: transition time
   * @param [in] state: transition state
   * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
   *                      @see PreComputation class documentation.
   * @return mapped state after transition
   */
  virtual vector_t computeJumpMap(scalar_t time, const vector_t& state, const PreComputation& preComp);

  /**
   * Computes the flow map of a system with exogenous input.
   *
   * @note This method calls the internal preComputation request() callback and the virtual
   *       computeFlowMap() with the preComputation as parameter.
   *       This interface is used by Rollout and SensitivityIntegrator.
   */
  vector_t computeFlowMap(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * State map at the transition time
   *
   * @note This method calls the internal preComputation requestPreJump() callback and the virtual
   *       computeJumpMap() with the preComputation as parameter.
   *       This interface is used by Rollout.
   */
  vector_t computeJumpMap(scalar_t time, const vector_t& state) override final;

  /** Get the pre-computation module */
  const PreComputation& getPreComputation() const { return *preCompPtr_; }

 protected:
  /**
   * Copy constructor
   *
   * @note Keeps the same controller pointer.
   * @note Clones the pre-computation object.
   */
  ControlledSystemBase(const ControlledSystemBase& other);

  std::unique_ptr<PreComputation> preCompPtr_;  //! pointer to pre-computation module

 private:
  ControllerBase* controllerPtr_ = nullptr;  //! pointer to controller
};
```

非自动系统动态的基类，也就是说有输入的系统.

```CPP
/**
  * Computes the flow map of a system with exogenous input.
  *
  * @param [in] t: The current time.
  * @param [in] x: The current state.
  * @param [in] u: The current input.
  * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
  *                      @see PreComputation class documentation.
  * @return The state time derivative.
  */
virtual vector_t computeFlowMap(scalar_t t, const vector_t& x, const vector_t& u, const PreComputation& preComp) = 0;
```

纯虚函数`computeFlowMap`定义了系统动态

$$
\dot{\mathbf x}(t) = \mathbf f_i(\mathbf x(t), \mathbf u(t), t)
$$

```CPP
/**
  * State map at the transition time
  *
  * @param [in] time: transition time
  * @param [in] state: transition state
  * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
  *                      @see PreComputation class documentation.
  * @return mapped state after transition
  */
virtual vector_t computeJumpMap(scalar_t time, const vector_t& state, const PreComputation& preComp);
```

虚函数`computeJumpMap`定义了系统切换动态

$$
\mathbf x(t_{i+1}^+) = \mathbf j(\mathbf x(t_{i+1}))
$$

```CPP
/**
  * Computes the flow map of a system.
  *
  * @param [in] t: The current time.
  * @param [in] x: The current state.
  * @return The state time derivative.
  */
vector_t computeFlowMap(scalar_t t, const vector_t& x) override final;

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
vector_t ControlledSystemBase::computeFlowMap(scalar_t t, const vector_t& x) {
  assert(controllerPtr_ != nullptr);
  const vector_t u = controllerPtr_->computeInput(t, x);
  return computeFlowMap(t, x, u);
}
```

`OdeBase`基类函数的覆盖，是由受控系统模拟自动系统的函数,可以看到，会计算当前的控制输入，调用有控制输入的`computeFlowMap`

### SystemDynamicsBase

```CPP
/**
 * The system dynamics and linearization class.
 * The linearized system flow map is defined as: \n
 * \f$ dx/dt = A(t) \delta x + B(t) \delta u \f$ \n
 * The linearized system jump map is defined as: \n
 * \f$ x^+ = G \delta x + H \delta u \f$ \n
 */
class SystemDynamicsBase : public ControlledSystemBase {
 public:
  /**
   * Constructor
   *
   * @param [in] preComputation: The (optional) pre-computation module, internally keeps a copy.
   *                             @see PreComputation class documentation.
   */
  explicit SystemDynamicsBase(const PreComputation& preComputation = PreComputation());

  /** Default destructor */
  ~SystemDynamicsBase() override = default;

  /** Clone */
  SystemDynamicsBase* clone() const override = 0;

  /**
   * Computes the linear approximation.
   *
   * @param [in] t: The current time.
   * @param [in] x: The current state.
   * @param [in] u: The current input.
   * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
   *                      @see PreComputation class documentation.
   * @return The state time derivative linear approximation.
   */
  virtual VectorFunctionLinearApproximation linearApproximation(scalar_t t, const vector_t& x, const vector_t& u,
                                                                const PreComputation& preComp) = 0;

  /** Computes the jump map linear approximation.
   *
   * @param [in] t: The current time.
   * @param [in] x: The current state.
   * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
   *                      @see PreComputation class documentation.
   * @return The linear approximation of the mapped state after transition
   */
  virtual VectorFunctionLinearApproximation jumpMapLinearApproximation(scalar_t t, const vector_t& x, const PreComputation& preComp);

  /** Computes the guard surfaces linear approximation */
  virtual VectorFunctionLinearApproximation guardSurfacesLinearApproximation(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * Get partial time derivative of the system flow map.
   * \f$ \frac{\partial f}{\partial t}  \f$.
   *
   * @return \f$ \frac{\partial f}{\partial t} \f$ matrix, size \f$ n_x \f$.
   */
  virtual vector_t flowMapDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * Get partial time derivative of the system jump map.
   * \f$ \frac{\partial g}{\partial t}  \f$.
   *
   * @return \f$ \frac{\partial g}{\partial t} \f$ matrix.
   */
  virtual vector_t jumpMapDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * Get at a given operating point the derivative of the guard surfaces w.r.t. input vector.
   *
   * @return Derivative of the guard surfaces w.r.t. time.
   */
  virtual vector_t guardSurfacesDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * Get at a given operating point the covariance of the dynamics.
   *
   * @return The covariance of the dynamics.
   */
  virtual matrix_t dynamicsCovariance(scalar_t t, const vector_t& x, const vector_t& u);

  /**
   * Computes the flow map linear approximation.
   *
   * @note This method updates the internal preComputation with the request() callback and passes it
   *       to the virtual linearApproximation() with the preComputation parameter.
   *       This interface is used by SensitivityIntegrator.
   */
  VectorFunctionLinearApproximation linearApproximation(scalar_t t, const vector_t& x, const vector_t& u);

  /** Computes the jump map linear approximation.
   *
   * @note This method updates the internal preComputation with the requestPreJump() callback and
   *       passes it to the virtual jumpMapLinearApproximation() with the preComputation parameter.
   */
  VectorFunctionLinearApproximation jumpMapLinearApproximation(scalar_t t, const vector_t& x);

 protected:
  /** Copy constructor */
  SystemDynamicsBase(const SystemDynamicsBase& other);
};
```

定义系统动态的基类.继承并实现虚函数即可定义系统动态.包含系统动态的泰勒估计.

#### linearApproximation

$$
\begin{align*}
\frac{d\delta \textbf{x}}{dt} &= A(t) \delta\textbf{x} + B(t) \delta\textbf{u} \\
A(t) &= \nabla_\textbf{x}f(\bar{\textbf{x}},\bar{\textbf{u}},t) \\
B(t) &= \nabla_\textbf{u}f(\bar{\textbf{x}},\bar{\textbf{u}},t)
\end{align*}
$$

```CPP
/**
  * Computes the linear approximation.
  *
  * @param [in] t: The current time.
  * @param [in] x: The current state.
  * @param [in] u: The current input.
  * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
  *                      @see PreComputation class documentation.
  * @return The state time derivative linear approximation.
  */
virtual VectorFunctionLinearApproximation linearApproximation(scalar_t t, const vector_t& x, const vector_t& u,
                                                              const PreComputation& preComp) = 0;
```

纯虚函数表示计算当前点的系统状态线性估计.

#### jumpMapLinearApproximation

$$
x^+ = G \delta x
$$

```CPP
/** Computes the jump map linear approximation.
  *
  * @param [in] t: The current time.
  * @param [in] x: The current state.
  * @param [in] preComp: pre-computation module, safely ignore this parameter if not used.
  *                      @see PreComputation class documentation.
  * @return The linear approximation of the mapped state after transition
  */
virtual VectorFunctionLinearApproximation jumpMapLinearApproximation(scalar_t t, const vector_t& x, const PreComputation& preComp);
```

虚函数表示计算当前点系统切换状态的线性估计.

### SystemDynamicsBaseAD类

```CPP
/**
 * The system dynamics Base with Algorithmic Differentiation (i.e. Auto Differentiation).
 * The linearized system flow map is defined as: \n
 * \f$ dx/dt = A(t) \delta x + B(t) \delta u \f$ \n
 * The linearized system jump map is defined as: \n
 * \f$ x^+ = G \delta x + H \delta u \f$ \n
 */
class SystemDynamicsBaseAD : public SystemDynamicsBase {
 public:
  /** Constructor */
  SystemDynamicsBaseAD();

  /** Default destructor */
  ~SystemDynamicsBaseAD() override = default;

  /**
   * Initializes model libraries
   *
   * @param stateDim : state vector dimension.
   * @param inputDim : input vector dimension.
   * @param modelName : name of the generate model library
   * @param modelFolder : folder to save the model library files to
   * @param recompileLibraries : If true, always compile the model library, else try to load existing library if available.
   * @param verbose : print information.
   */
  void initialize(size_t stateDim, size_t inputDim, const std::string& modelName, const std::string& modelFolder = "/tmp/ocs2",
                  bool recompileLibraries = true, bool verbose = true);

  vector_t computeFlowMap(scalar_t t, const vector_t& x, const vector_t& u, const PreComputation& preComputation) final;

  vector_t computeJumpMap(scalar_t t, const vector_t& x, const PreComputation& preComputation) final;

  vector_t computeGuardSurfaces(scalar_t t, const vector_t& x) final;

  VectorFunctionLinearApproximation linearApproximation(scalar_t t, const vector_t& x, const vector_t& u,
                                                        const PreComputation& preComputation) final;

  VectorFunctionLinearApproximation jumpMapLinearApproximation(scalar_t t, const vector_t& x, const PreComputation& preComputation) final;

  VectorFunctionLinearApproximation guardSurfacesLinearApproximation(scalar_t t, const vector_t& x, const vector_t& u) final;

  /** @note: Requires linear approximation to be called before */
  vector_t flowMapDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u) final;

  /** @note: Requires jump map linear approximation to be called before */
  vector_t jumpMapDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u) final;

  /** @note: Requires guard surfaces linear approximation to be called before */
  vector_t guardSurfacesDerivativeTime(scalar_t t, const vector_t& x, const vector_t& u) final;

 protected:
  /** Copy constructor */
  SystemDynamicsBaseAD(const SystemDynamicsBaseAD& rhs);

  /**
   * Interface method to the state flow map of the hybrid system. This method should be implemented by the derived class.
   *
   * @param [in] time: time.
   * @param [in] state: state vector.
   * @param [in] input: input vector.
   * @param [in] parameters: parameter vector.
   * @return state vector time derivative.
   */
  virtual ad_vector_t systemFlowMap(ad_scalar_t time, const ad_vector_t& state, const ad_vector_t& input,
                                    const ad_vector_t& parameters) const = 0;

  /**
   * Gets the parameters of the system flow map
   *
   * @param [in] time: Current time.
   * @return The parameters to be set in the flow map at the start of the horizon
   */
  virtual vector_t getFlowMapParameters(scalar_t time, const PreComputation& /* preComputation */) const { return vector_t(0); }

  /**
   * Number of parameters for system flow map.
   *
   * @return number of parameters
   */
  virtual size_t getNumFlowMapParameters() const { return 0; }

  /**
   * Interface method to the state jump map of the hybrid system. This method can be implemented by the derived class.
   *
   * @param [in] time: time.
   * @param [in] state: state vector.
   * @param [in] parameters: parameter vector.
   * @return jumped state.
   */
  virtual ad_vector_t systemJumpMap(ad_scalar_t time, const ad_vector_t& state, const ad_vector_t& parameters) const;

  /**
   * Gets the parameters of the jump map
   *
   * @param [in] time: Current time.
   * @return The parameters to be set in the jump map
   */
  virtual vector_t getJumpMapParameters(scalar_t time, const PreComputation& /* preComputation */) const { return vector_t(0); }

  /**
   * Number of parameters for jump map.
   *
   * @return number of parameters
   */
  virtual size_t getNumJumpMapParameters() const { return 0; }

  /**
   * Interface method to the guard surfaces. This method can be implemented by the derived class.
   *
   * @param [in] time: time.
   * @param [in] state: state.
   * @param [in] input: input vector
   * @param [in] parameters: parameter vector.
   * @return A vector of guard surfaces values
   */
  virtual ad_vector_t systemGuardSurfaces(ad_scalar_t time, const ad_vector_t& state, const ad_vector_t& parameters) const;

  /**
   * Gets the parameters of the guard surfaces
   *
   * @param [in] time: Current time.
   * @return The parameters to be set in the guard surfaces
   */
  virtual vector_t getGuardSurfacesParameters(scalar_t time) const { return vector_t(0); }

  /**
   * Number of parameters for guard surfaces.
   *
   * @return number of parameters
   */
  virtual size_t getNumGuardSurfacesParameters() const { return 0; }

 private:
  std::unique_ptr<CppAdInterface> flowMapADInterfacePtr_;
  std::unique_ptr<CppAdInterface> jumpMapADInterfacePtr_;
  std::unique_ptr<CppAdInterface> guardSurfacesADInterfacePtr_;

  vector_t tapedTimeStateInput_;
  vector_t tapedTimeState_;

  /** Cached jacobians for time derivative */
  matrix_t flowJacobian_;
  matrix_t jumpJacobian_;
  matrix_t guardJacobian_;
};
```

这是自动微分的类，只需要继承并实现`systemFlowMap`与`systemJumpMap`即可进行自动微分，求解线性化点.

## 声明代价函数Cost

### StateInputCost

```CPP
/** State-input cost term */
class StateInputCost {
 public:
  StateInputCost() = default;
  virtual ~StateInputCost() = default;
  virtual StateInputCost* clone() const = 0;

  /** Check if cost term is active */
  virtual bool isActive(scalar_t time) const { return true; }

  /** Get cost term value */
  virtual scalar_t getValue(scalar_t time, const vector_t& state, const vector_t& input, const TargetTrajectories& targetTrajectories,
                            const PreComputation& preComp) const = 0;

  /** Get cost term quadratic approximation */
  virtual ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                         const TargetTrajectories& targetTrajectories,
                                                                         const PreComputation& preComp) const = 0;

 protected:
  StateInputCost(const StateInputCost& rhs) = default;
};
```

这是一个基类，继承它可以定义各式各样的状态-输入代价函数.

`clone`表示克隆当前代价函数

`getValue`纯虚函数表示获取当前时间，状态，输入，目标轨迹下的代价值,`preComp`用于加速计算.

`getQuadraticApproximation`表示获取时间，状态，输入，目标轨迹下代价函数的二阶泰勒估计.

#### QuadraticStateInputCost

```CPP
/** Quadratic state-input cost term */
class QuadraticStateInputCost : public StateInputCost {
 public:
  /**
   * Constructor for the quadratic cost function defined as the following:
   * \f$ L = 0.5(x-x_{n})' Q (x-x_{n}) + 0.5(u-u_{n})' R (u-u_{n}) + (u-u_{n})' P (x-x_{n}) \f$
   * @param [in] Q: \f$ Q \f$
   * @param [in] R: \f$ R \f$
   * @param [in] P: \f$ P \f$
   */
  QuadraticStateInputCost(matrix_t Q, matrix_t R, matrix_t P = matrix_t());
  ~QuadraticStateInputCost() override = default;
  QuadraticStateInputCost* clone() const override;

  /** Get cost term value */
  scalar_t getValue(scalar_t time, const vector_t& state, const vector_t& input, const TargetTrajectories& targetTrajectories,
                    const PreComputation&) const final;

  /** Get cost term quadratic approximation */
  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                 const TargetTrajectories& targetTrajectories,
                                                                 const PreComputation&) const final;

 protected:
  QuadraticStateInputCost(const QuadraticStateInputCost& rhs) = default;

  /** Computes the state-input deviation pair around the nominal state and input.
   * This method can be overwritten if desiredTrajectory has a different dimensions. */
  virtual std::pair<vector_t, vector_t> getStateInputDeviation(scalar_t time, const vector_t& state, const vector_t& input,
                                                               const TargetTrajectories& targetTrajectories) const;

 private:
  matrix_t Q_;
  matrix_t R_;
  matrix_t P_;
};
```

二次型代价函数

$$
L = \frac{1}{2}(\textbf{x}-\textbf{x}_n)^TQ(\textbf{x} - \textbf{x}_n) + \frac{1}{2}(\textbf{u} - \textbf{u}_n)^TR(\textbf{u} - \textbf{u}_n)
+ (\textbf{u} - \textbf{u}_n)^TP(\textbf{x}-\textbf{x}_n)
$$

##### clone

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
QuadraticStateInputCost* QuadraticStateInputCost::clone() const {
  return new QuadraticStateInputCost(*this);
}
```

##### getValue

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
scalar_t QuadraticStateInputCost::getValue(scalar_t time, const vector_t& state, const vector_t& input,
                                           const TargetTrajectories& targetTrajectories, const PreComputation&) const {
  vector_t stateDeviation, inputDeviation;
  std::tie(stateDeviation, inputDeviation) = getStateInputDeviation(time, state, input, targetTrajectories);

  if (P_.size() == 0) {
    return 0.5 * stateDeviation.dot(Q_ * stateDeviation) + 0.5 * inputDeviation.dot(R_ * inputDeviation);
  } else {
    return 0.5 * stateDeviation.dot(Q_ * stateDeviation) + 0.5 * inputDeviation.dot(R_ * inputDeviation) +
           inputDeviation.dot(P_ * stateDeviation);
  }
}

std::pair<vector_t, vector_t> QuadraticStateInputCost::getStateInputDeviation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                              const TargetTrajectories& targetTrajectories) const {
  const vector_t stateDeviation = state - targetTrajectories.getDesiredState(time);
  const vector_t inputDeviation = input - targetTrajectories.getDesiredInput(time);
  return {stateDeviation, inputDeviation};
}
```

二次型函数求值

##### getQuadraticApproximation

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
ScalarFunctionQuadraticApproximation QuadraticStateInputCost::getQuadraticApproximation(scalar_t time, const vector_t& state,
                                                                                        const vector_t& input,
                                                                                        const TargetTrajectories& targetTrajectories,
                                                                                        const PreComputation&) const {
  vector_t stateDeviation, inputDeviation;
  std::tie(stateDeviation, inputDeviation) = getStateInputDeviation(time, state, input, targetTrajectories);

  ScalarFunctionQuadraticApproximation L;
  L.dfdxx = Q_;
  L.dfduu = R_;
  L.dfdx.noalias() = Q_ * stateDeviation;
  L.dfdu.noalias() = R_ * inputDeviation;
  L.f = 0.5 * stateDeviation.dot(L.dfdx) + 0.5 * inputDeviation.dot(L.dfdu);

  if (P_.size() == 0) {
    L.dfdux.setZero(input.size(), state.size());

  } else {
    const vector_t pDeviation = P_ * stateDeviation;
    L.f += inputDeviation.dot(pDeviation);
    L.dfdu += pDeviation;
    L.dfdx.noalias() += P_.transpose() * inputDeviation;
    L.dfdux = P_;
  }

  return L;
}
```

获取二阶泰勒展开

### StateCost

```CPP
/** State-only cost term */
class StateCost {
 public:
  StateCost() = default;
  virtual ~StateCost() = default;
  virtual StateCost* clone() const = 0;

  /** Check if cost term is active */
  virtual bool isActive(scalar_t time) const { return true; }

  /** Get cost term value */
  virtual scalar_t getValue(scalar_t time, const vector_t& state, const TargetTrajectories& targetTrajectories,
                            const PreComputation& preComp) const = 0;

  /** Get cost term quadratic approximation */
  virtual ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state,
                                                                         const TargetTrajectories& targetTrajectories,
                                                                         const PreComputation& preComp) const = 0;

 protected:
  StateCost(const StateCost& rhs) = default;
};
```

这是一个通用基类，继承它可以定义各式各样的状态代价函数.

#### QuadraticStateCost

```CPP
/** Quadratic state-only cost term */
class QuadraticStateCost : public StateCost {
 public:
  /**
   * Constructor for the quadratic cost function defined as the following:
   * \f$ \l = 0.5(x-x_{n})' Q (x-x_{n}) \f$.
   * @param [in] Q: \f$ Q \f$
   */
  explicit QuadraticStateCost(matrix_t Q);
  ~QuadraticStateCost() override = default;
  QuadraticStateCost* clone() const override;

  /** Get cost term value */
  scalar_t getValue(scalar_t time, const vector_t& state, const TargetTrajectories& targetTrajectories, const PreComputation&) const final;

  /** Get cost term quadratic approximation */
  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state,
                                                                 const TargetTrajectories& targetTrajectories,
                                                                 const PreComputation&) const final;

 protected:
  QuadraticStateCost(const QuadraticStateCost& rhs) = default;

  /** Computes the state deviation for the nominal state.
   * This method can be overwritten if desiredTrajectory has a different dimensions. */
  virtual vector_t getStateDeviation(scalar_t time, const vector_t& state, const TargetTrajectories& targetTrajectories) const;

 private:
  matrix_t Q_;
};
```

类似的，是一个二次型代价函数.

$$
L = \frac{1}{2}(\textbf{x} - \textbf{x}_n)^TQ(\textbf{x} - \textbf{x}_n)
$$

## 声明约束Constraint

约束分为硬约束(hard constraints或constraints)与软约束(soft contraints).处理方法不同.

```CPP
/* Soft constraints */
/** Intermediate soft constraint penalty */
std::unique_ptr<StateInputCostCollection> softConstraintPtr;
/** Intermediate state-only soft constraint penalty */
std::unique_ptr<StateCostCollection> stateSoftConstraintPtr;
/** Pre-jump soft constraint penalty */
std::unique_ptr<StateCostCollection> preJumpSoftConstraintPtr;
/** Final soft constraint penalty */
std::unique_ptr<StateCostCollection> finalSoftConstraintPtr;
```

可以看到，软约束实际上是代价函数

硬约束需要声明约束的次数，线性还是二次的泰勒近似.

```CPP
enum class ConstraintOrder { Linear, Quadratic };
```

### StateConstraint类

```CPP
/** State-only constraint function base class */
class StateConstraint {
 public:
  explicit StateConstraint(ConstraintOrder order) : order_(order) {}
  virtual ~StateConstraint() = default;
  virtual StateConstraint* clone() const = 0;

  /** Get the constraint order (Linear or Quadratic) */
  constexpr ConstraintOrder getOrder() const { return order_; };

  /** Check constraint activity */
  virtual bool isActive(scalar_t time) const { return true; }

  /** Get the size of the constraint vector at given time */
  virtual size_t getNumConstraints(scalar_t time) const = 0;

  /** Get the constraint vector value */
  virtual vector_t getValue(scalar_t time, const vector_t& state, const PreComputation& preComp) const = 0;

  /** Get the constraint linear approximation */
  virtual VectorFunctionLinearApproximation getLinearApproximation(scalar_t time, const vector_t& state,
                                                                   const PreComputation& preComp) const {
    if (order_ == ConstraintOrder::Linear) {
      throw std::runtime_error("[StateConstraint] Linear approximation not implemented!");
    } else {
      throw std::runtime_error("[StateConstraint] The class only provides Quadratic approximation! call getQuadraticApproximation()");
    }
  }

  /** Get the constraint quadratic approximation */
  virtual VectorFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state,
                                                                         const PreComputation& preComp) const {
    if (order_ == ConstraintOrder::Quadratic) {
      throw std::runtime_error("[StateConstraint] Quadratic approximation not implemented!");
    } else {
      throw std::runtime_error("[StateConstraint] The class only provides Linear approximation! call getLinearApproximation()");
    }
  }

 protected:
  StateConstraint(const StateConstraint& rhs) = default;

 private:
  ConstraintOrder order_;
};
```

这是纯状态约束的通用基类,

`getValue`获取在当前时间，当前状态下的约束的值,如果是等式约束,就是${\mathbf g_2}_i(\mathbf x(t), t)$的值，不等式约束就是$h_i(\mathbf x(t), t)$的值.

`getNumConstraints`获取当前这个类有多少个约束,就是`vector_t`的长度.

### LinearStateConstraint类

```CPP
/**
 * Linear state-only constraint
 */
class LinearStateConstraint : public StateConstraint {
 public:
  /**
   * Constructor
   *
   * @param [in] h: Constant term in F * x + h = 0
   * @param [in] F: x factor in F * x + h = 0
   */
  LinearStateConstraint(vector_t h, matrix_t F);

  ~LinearStateConstraint() override = default;

  LinearStateConstraint* clone() const override;

  size_t getNumConstraints(scalar_t time) const final;

  vector_t getValue(scalar_t t, const vector_t& x, const PreComputation& /* preComputation */) const final;

  VectorFunctionLinearApproximation getLinearApproximation(scalar_t t, const vector_t& x,
                                                           const PreComputation& /* preComputation */) const final;

 public:
  vector_t h_; /**< State only constraint */
  matrix_t F_; /**< State only constraint derivative wrt. state */
};
```

这是一个简单的线性纯状态的约束

对于等式约束就是

$$
F\textbf{x} + \textbf{h} = 0
$$

对于不等式约束就是

$$
F\textbf{x} + \textbf{h} \geq 0
$$

#### getValue

```CPP
vector_t LinearStateConstraint::getValue(scalar_t t, const vector_t& x, const PreComputation&) const {
  vector_t g = h_;
  g.noalias() += F_ * x;
  return g;
}
```

获取值

#### getLinearApproximation

```CPP
VectorFunctionLinearApproximation LinearStateConstraint::getLinearApproximation(scalar_t t, const vector_t& x,
                                                                                const PreComputation&) const {
  VectorFunctionLinearApproximation g;
  g.f = h_;
  g.f.noalias() += F_ * x;
  g.dfdx = F_;
  return g;
}
```

### StateInputConstraint类

```CPP
class StateInputConstraint {
 public:
  explicit StateInputConstraint(ConstraintOrder order) : order_(order) {}
  virtual ~StateInputConstraint() = default;
  virtual StateInputConstraint* clone() const = 0;

  /** Get the constraint order (Linear or Quadratic) */
  constexpr ConstraintOrder getOrder() const { return order_; };

  /** Check constraint activity */
  virtual bool isActive(scalar_t time) const { return true; }

  /** Get the size of the constraint vector at given time */
  virtual size_t getNumConstraints(scalar_t time) const = 0;

  /** Get the constraint vector value */
  virtual vector_t getValue(scalar_t time, const vector_t& state, const vector_t& input, const PreComputation& preComp) const = 0;

  /** Get the constraint linear approximation */
  virtual VectorFunctionLinearApproximation getLinearApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                   const PreComputation& preComp) const {
    if (order_ == ConstraintOrder::Linear) {
      throw std::runtime_error("[StateInputConstraint] Linear approximation not implemented!");
    } else {
      throw std::runtime_error("[StateInputConstraint] The class only provides Quadratic approximation! call getQuadraticApproximation()");
    }
  }

  /** Get the constraint quadratic approximation */
  virtual VectorFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                         const PreComputation& preComp) const {
    if (order_ == ConstraintOrder::Quadratic) {
      throw std::runtime_error("[StateConstraint] Quadratic approximation not implemented!");
    } else {
      throw std::runtime_error("[StateConstraint] The class only provides Linear approximation! call getLinearApproximation()");
    }
  }

 protected:
  StateInputConstraint(const StateInputConstraint& rhs) = default;

 private:
  ConstraintOrder order_;
};
```

状态-输入约束.

### 自动微分支持

```CPP
class StateInputConstraintCppAd : public StateInputConstraint;
class StateConstraintCppAd : public StateConstraint;
```

### LinearStateInputConstraint类

```CPP
/**
 * Linear state-input constraint
 */
class LinearStateInputConstraint : public StateInputConstraint {
 public:
  /**
   * Constructor
   *
   * @param[in] e: Constant term in C * x + D * u + e = 0
   * @param[in] C: x factor in C * x + D * u + e = 0
   * @param[in] D: u factor in C * x + D * u + e = 0
   */
  LinearStateInputConstraint(vector_t e, matrix_t C, matrix_t D);

  ~LinearStateInputConstraint() override = default;

  LinearStateInputConstraint* clone() const override;

  size_t getNumConstraints(scalar_t time) const final;

  vector_t getValue(scalar_t t, const vector_t& x, const vector_t& u, const PreComputation& /* preComputation */) const final;

  VectorFunctionLinearApproximation getLinearApproximation(scalar_t t, const vector_t& x, const vector_t& u,
                                                           const PreComputation& /* preComputation */) const final;

 public:
  vector_t e_; /**< State input constraint */
  matrix_t C_; /**< State input constraint derivative wrt. state */
  matrix_t D_; /**< State input constraint derivative wrt. input */
};
```

这是一个简单的线性状态输入的约束

对于等式约束就是

$$
C\textbf{x} + D\textbf{u} + \textbf{e} = 0
$$

对于不等式约束就是

$$
C\textbf{x} + D\textbf{u} + \textbf{e} \geq 0
$$

#### getValue

```CPP
vector_t LinearStateInputConstraint::getValue(scalar_t t, const vector_t& x, const vector_t& u, const PreComputation&) const {
  vector_t g = e_;
  g.noalias() += C_ * x;
  g.noalias() += D_ * u;
  return g;
}
```

#### getLinearApproximation

```CPP
VectorFunctionLinearApproximation LinearStateInputConstraint::getLinearApproximation(scalar_t t, const vector_t& x, const vector_t& u,
                                                                                     const PreComputation&) const {
  VectorFunctionLinearApproximation g;
  g.f = e_;
  g.f.noalias() += C_ * x;
  g.f.noalias() += D_ * u;
  g.dfdx = C_;
  g.dfdu = D_;
  return g;
}
```

## 声明拉格朗日约束AugmentedLagrang

增广拉格朗日约束是接受拉格朗日乘子，约束违反，时间的约束函数.

### 工厂函数

```CPP
/**
 * Factory function for state augmented Lagrangian.
 * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
 * @param [in] penaltyPtrArray: An array of pointers to the penalty function on the constraint.
 */
std::unique_ptr<StateAugmentedLagrangian> create(std::unique_ptr<StateConstraint> constraintPtr,
                                                 std::vector<std::unique_ptr<augmented::AugmentedPenaltyBase>> penaltyPtrArray) {
  return std::make_unique<StateAugmentedLagrangian>(std::move(constraintPtr), std::move(penaltyPtrArray));
}

                                                 

/**
 * Factory function for state augmented Lagrangian.
 * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
 * @param [in] penaltyPtr: A pointer to the penalty function on the constraint.
 */
std::unique_ptr<StateAugmentedLagrangian> create(std::unique_ptr<StateConstraint> constraintPtr,
                                                 std::unique_ptr<augmented::AugmentedPenaltyBase> penaltyPtr) {
  return std::make_unique<StateAugmentedLagrangian>(std::move(constraintPtr), std::move(penaltyPtr));
}

/**
 * Factory function for state-input augmented Lagrangian.
 * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
 * @param [in] penaltyPtrArray: An array of pointers to the penalty function on the constraint.
 */
std::unique_ptr<StateInputAugmentedLagrangian> create(std::unique_ptr<StateInputConstraint> constraintPtr,
                                                      std::vector<std::unique_ptr<augmented::AugmentedPenaltyBase>> penaltyPtrArray){
  return std::make_unique<StateInputAugmentedLagrangian>(std::move(constraintPtr), std::move(penaltyPtrArray));
}
/**
 * Factory function for state-input augmented Lagrangian.
 * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
 * @param [in] penaltyPtr: A pointer to the penalty function on the constraint.
 */
std::unique_ptr<StateInputAugmentedLagrangian> create(std::unique_ptr<StateInputConstraint> constraintPtr,
                                                      std::unique_ptr<augmented::AugmentedPenaltyBase> penaltyPtr) {
  return std::make_unique<StateInputAugmentedLagrangian>(std::move(constraintPtr), std::move(penaltyPtr));
}
```

工厂函数接收约束函数，罚函数，生成对应的增广拉格朗日约束.

### Multiplier类

```CPP
/** The penalty and Lagrangian multipliers associated to a constraint vector of size Multiplier::lagrangian.size() */
struct Multiplier {
  Multiplier() : Multiplier(0.0, vector_t()) {}
  Multiplier(scalar_t penaltyArg, vector_t lagrangianArg) : penalty(penaltyArg), lagrangian(std::move(lagrangianArg)) {}

  scalar_t penalty;
  vector_t lagrangian;
};
```

保存了拉格朗日乘子.

### LagrangianMetrics类

```CPP
/** The structure contains a term's constraint vector and its associated penalty */
struct LagrangianMetrics {
  LagrangianMetrics() : LagrangianMetrics(0.0, vector_t()) {}
  LagrangianMetrics(scalar_t penaltyArg, vector_t constraintArg) : penalty(penaltyArg), constraint(std::move(constraintArg)) {}

  scalar_t penalty;
  vector_t constraint;
};
```

保存了约束值，与相关的惩罚值.

### MultidimensionalPenalty类

```CPP

/**
 *   A helper class that implements the penalty for multidimensional constraint
 *   \f$ h_i(x, u) \quad \forall  i \in [1,..,M] \f$
 *
 *   penalty(t, x, u) = \f$ \sum_{i=1}^{M} p(t, h_i(x, u)) \f$
 *
 *   This class uses the chain rule to compute the second-order approximation of the constraint-penalty. In the case that the
 *   second-order approximation of constraint is not provided, it employs a Gauss-Newton approximation technique which only
 *   relies on the first-order approximation. In general, the penalty function can be a function of time.
 */
class MultidimensionalPenalty final {
 public:
  /**
   * Constructor
   * @note This imposes a fixed number of constraints, where the corresponding penalty function in the array is applied.
   * @param [in] penaltyPtrArray: An array of pointers to the penalty function on the constraint.
   */
  template <class PenaltyType>
  MultidimensionalPenalty(std::vector<std::unique_ptr<PenaltyType>> penaltyPtrArray);

  /**
   * Constructor with s single penalty function
   * @note This allows a varying number of constraints and uses the same penalty function for each constraint.
   * @param [in] penaltyPtr: A pointer to the penalty function on the constraint.
   */
  template <class PenaltyType>
  MultidimensionalPenalty(std::unique_ptr<PenaltyType> penaltyPtr);

  /** Default destructor */
  ~MultidimensionalPenalty() = default;

  /** Copy constructor */
  MultidimensionalPenalty(const MultidimensionalPenalty& other);

  /**
   * Get the penalty cost.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: Vector of inequality constraint values.
   * @return Penalty: The penalty cost.
   */
  scalar_t getValue(scalar_t t, const vector_t& h, const vector_t* l = nullptr) const;

  /**
   * Get the derivative of the penalty cost.
   * Implements the chain rule between the inequality constraint and penalty function.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: The constraint linear approximation.
   * @return The penalty cost quadratic approximation.
   */
  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t t, const VectorFunctionLinearApproximation& h,
                                                                 const vector_t* l = nullptr) const;

  /**
   * Get the derivative of the penalty cost.
   * Implements the chain rule between the inequality constraint and penalty function.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: The constraint quadratic approximation.
   * @return The penalty cost quadratic approximation.
   */
  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t t, const VectorFunctionQuadraticApproximation& h,
                                                                 const vector_t* l = nullptr) const;

  /**
   * Updates the Lagrange multipliers.
   *
   * @param [in] t: The time stamp.
   * @param [in] l: The Lagrange multipliers.
   * @param [in] h: The vector of constraint values.
   * @return updated Lagrange multipliers.
   */
  vector_t updateMultipliers(scalar_t t, const vector_t& h, const vector_t& l) const;

  /**
   * Initializes the Lagrange multipliers.
   *
   * @param [in] numConstraints: Number of constraints associated to this penalty.
   * @return Initial Lagrange multipliers.
   */
  vector_t initializeMultipliers(size_t numConstraints) const;

 private:
  std::tuple<scalar_t, vector_t, vector_t> getPenaltyValue1stDev2ndDev(scalar_t t, const vector_t& h, const vector_t* l) const;

  std::vector<std::unique_ptr<augmented::AugmentedPenaltyBase>> penaltyPtrArray_;
};
```

实现了一个多个约束下的罚函数类

$$
h_i(\textbf{x}, \textbf{u}) \quad \forall  i \in [1,..,M] \\
p(t,\textbf{x},\textbf{u}) = \sum_{i=1}^{M} p(t, h_i(\textbf{x}, \textbf{u}))
$$

其实就是实现了链式法则，

#### getValue

```CPP
scalar_t MultidimensionalPenalty::getValue(scalar_t t, const vector_t& h, const vector_t* l) const {
  const auto numConstraints = h.rows();
  assert(penaltyPtrArray_.size() == 1 || penaltyPtrArray_.size() == numConstraints);

  scalar_t penalty = 0;
  for (size_t i = 0; i < numConstraints; i++) {
    const auto& penaltyTerm = (penaltyPtrArray_.size() == 1) ? penaltyPtrArray_[0] : penaltyPtrArray_[i];
    penalty += penaltyTerm->getValue(t, getMultiplier(l, i), h(i));
  }

  return penalty;
}
```

遍历当前罚函数，获取总的罚函数值.

#### updateMultipliers与initializeMultipliers

```CPP
vector_t MultidimensionalPenalty::updateMultipliers(scalar_t t, const vector_t& h, const vector_t& l) const {
  const size_t numConstraints = h.size();
  assert(l.size() == numConstraints);
  assert(penaltyPtrArray_.size() == 1 || penaltyPtrArray_.size() == numConstraints);

  vector_t updted_l(numConstraints);
  for (size_t i = 0; i < numConstraints; i++) {
    const auto& penaltyTerm = (penaltyPtrArray_.size() == 1) ? penaltyPtrArray_[0] : penaltyPtrArray_[i];
    updted_l(i) = penaltyTerm->updateMultiplier(t, l(i), h(i));
  }

  return updted_l;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
vector_t MultidimensionalPenalty::initializeMultipliers(size_t numConstraints) const {
  assert(penaltyPtrArray_.size() == 1 || penaltyPtrArray_.size() == numConstraints);

  vector_t l(numConstraints);
  for (size_t i = 0; i < numConstraints; i++) {
    const auto& penaltyTerm = (penaltyPtrArray_.size() == 1) ? penaltyPtrArray_[0] : penaltyPtrArray_[i];
    l(i) = penaltyTerm->initializeMultiplier();
  }

  return l;
}
```

初始化，更新拉格朗日乘子.

#### getQuadraticApproximation

```CPP
ScalarFunctionQuadraticApproximation MultidimensionalPenalty::getQuadraticApproximation(scalar_t t,
                                                                                        const VectorFunctionQuadraticApproximation& h,
                                                                                        const vector_t* l) const {
  const auto stateDim = h.dfdx.cols();
  const auto inputDim = h.dfdu.cols();
  const auto numConstraints = h.f.rows();

  scalar_t penaltyValue = 0.0;
  vector_t penaltyDerivative, penaltySecondDerivative;
  std::tie(penaltyValue, penaltyDerivative, penaltySecondDerivative) = getPenaltyValue1stDev2ndDev(t, h.f, l);
  const matrix_t penaltySecondDev_dhdx = penaltySecondDerivative.asDiagonal() * h.dfdx;

  // to make sure that dfdux in the state-only case has a right size
  ScalarFunctionQuadraticApproximation penaltyApproximation(stateDim, inputDim);

  penaltyApproximation.f = penaltyValue;
  penaltyApproximation.dfdx.noalias() = h.dfdx.transpose() * penaltyDerivative;
  penaltyApproximation.dfdxx.noalias() = h.dfdx.transpose() * penaltySecondDev_dhdx;
  for (size_t i = 0; i < numConstraints; i++) {
    penaltyApproximation.dfdxx.noalias() += penaltyDerivative(i) * h.dfdxx[i];
  }

  if (inputDim > 0) {
    penaltyApproximation.dfdu.noalias() = h.dfdu.transpose() * penaltyDerivative;
    penaltyApproximation.dfdux.noalias() = h.dfdu.transpose() * penaltySecondDev_dhdx;
    penaltyApproximation.dfduu.noalias() = h.dfdu.transpose() * penaltySecondDerivative.asDiagonal() * h.dfdu;
    for (size_t i = 0; i < numConstraints; i++) {
      penaltyApproximation.dfduu.noalias() += penaltyDerivative(i) * h.dfduu[i];
      penaltyApproximation.dfdux.noalias() += penaltyDerivative(i) * h.dfdux[i];
    }
  }

  return penaltyApproximation;
}
```

通过当前时间，以及约束函数的泰勒展开，获得这个罚函数的泰勒展开.

### StateAugmentedLagrangianInterface

```CPP
/** The base class for Augmented Lagrangian penalty of state constraint. */
class StateAugmentedLagrangianInterface {
 public:
  StateAugmentedLagrangianInterface() = default;
  virtual ~StateAugmentedLagrangianInterface() = default;
  virtual StateAugmentedLagrangianInterface* clone() const = 0;

  /** Check penalty's activity */
  virtual bool isActive(scalar_t time) const = 0;

  /** Get the size of the constraint vector at given time */
  virtual size_t getNumConstraints(scalar_t time) const = 0;

  /** Get the constraint and its penalty value */
  virtual LagrangianMetrics getValue(scalar_t time, const vector_t& state, const Multiplier& multiplier,
                                     const PreComputation& preComp) const = 0;

  /** Get the constraint's penalty quadratic approximation */
  virtual ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const Multiplier& multiplier,
                                                                         const PreComputation& preComp) const = 0;

  /** Update Lagrange/penalty multipliers and the penalty function value. */
  virtual std::pair<Multiplier, scalar_t> updateLagrangian(scalar_t time, const vector_t& state, const vector_t& constraint,
                                                           const Multiplier& multiplier) const = 0;

  /** Initialize Lagrange/penalty multipliers. */
  virtual Multiplier initializeLagrangian(scalar_t time) const = 0;

 protected:
  StateAugmentedLagrangianInterface(const StateAugmentedLagrangianInterface& rhs) = default;
};
```

纯状态拉格朗日约束的基类.

可以看到，获取当前约束函数值的`getValue`接收时间,当前状态，当前拉格朗日乘子作为参数.

### StateAugmentedLagrangian类

```CPP
/** The base class for Augmented Lagrangian penalty of state constraint. */
class StateAugmentedLagrangian final : public StateAugmentedLagrangianInterface {
 public:
  /**
   * Constructor.
   * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
   * @param [in] penaltyPtrArray: An array of pointers to the penalty function on the constraint.
   */
  StateAugmentedLagrangian(std::unique_ptr<StateConstraint> constraintPtr,
                           std::vector<std::unique_ptr<augmented::AugmentedPenaltyBase>> penaltyPtrArray);

  /**
   * Constructor.
   * @note This allows a varying number of constraints and uses the same penalty function for each constraint.
   * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
   * @param [in] penaltyPtr: A pointer to the penalty function on the constraint.
   */
  StateAugmentedLagrangian(std::unique_ptr<StateConstraint> constraintPtr, std::unique_ptr<augmented::AugmentedPenaltyBase> penaltyPtr);

  StateAugmentedLagrangian* clone() const override;
  bool isActive(scalar_t time) const override;
  size_t getNumConstraints(scalar_t time) const override;

  LagrangianMetrics getValue(scalar_t time, const vector_t& state, const Multiplier& multiplier,
                             const PreComputation& preComp) const override;

  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const Multiplier& multiplier,
                                                                 const PreComputation& preComp) const override;

  std::pair<Multiplier, scalar_t> updateLagrangian(scalar_t time, const vector_t& state, const vector_t& constraint,
                                                   const Multiplier& multiplier) const override;

  Multiplier initializeLagrangian(scalar_t time) const override;

  /** Gets the wrapped constraint. */
  template <typename Derived = StateConstraint>
  Derived& get() {
    static_assert(std::is_base_of<StateConstraint, Derived>::value, "Template argument must derive from StateConstraint!");
    return dynamic_cast<Derived&>(*constraintPtr_);
  }

 private:
  StateAugmentedLagrangian(const StateAugmentedLagrangian& other);

  std::unique_ptr<StateConstraint> constraintPtr_;
  MultidimensionalPenalty penalty_;
};
```

这是纯状态拉格朗日约束类.

#### getValue

```CPP
LagrangianMetrics StateAugmentedLagrangian::getValue(scalar_t time, const vector_t& state, const Multiplier& multiplier,
                                                     const PreComputation& preComp) const {
  const auto h = constraintPtr_->getValue(time, state, preComp);
  const auto p = multiplier.penalty * penalty_.getValue(time, h, &multiplier.lagrangian);
  return {p, h};
}
```

可以看到，首先调用约束函数，获取当前约束违反$h$,再调用惩罚函数，获取惩罚值，组合成`LagrangianMetrics`返回.

#### getQuadraticApproximation

```CPP
ScalarFunctionQuadraticApproximation StateAugmentedLagrangian::getQuadraticApproximation(scalar_t time, const vector_t& state,
                                                                                         const Multiplier& multiplier,
                                                                                         const PreComputation& preComp) const {
  switch (constraintPtr_->getOrder()) {
    case ConstraintOrder::Linear:
      return multiplier.penalty *
             penalty_.getQuadraticApproximation(time, constraintPtr_->getLinearApproximation(time, state, preComp), &multiplier.lagrangian);
    case ConstraintOrder::Quadratic:
      return multiplier.penalty * penalty_.getQuadraticApproximation(time, constraintPtr_->getQuadraticApproximation(time, state, preComp),
                                                                     &multiplier.lagrangian);
    default:
      throw std::runtime_error("[StateAugmentedLagrangian] Unknown constraint Order");
  }
}
```

获取当前时间，状态，拉格朗日乘子下的二次估计.

#### updateLagrangian

```CPP
std::pair<Multiplier, scalar_t> StateAugmentedLagrangian::updateLagrangian(scalar_t time, const vector_t& /*state*/,
                                                                           const vector_t& constraint, const Multiplier& multiplier) const {
  const Multiplier updatedMultiplier{multiplier.penalty, penalty_.updateMultipliers(time, constraint, multiplier.lagrangian)};
  const auto penalty = updatedMultiplier.penalty * penalty_.getValue(time, constraint, &updatedMultiplier.lagrangian);
  return {updatedMultiplier, penalty};
}
```

返回更新后的拉格朗日值，这些函数都是无状态的.

### StateInputAugmentedLagrangianInterface类

```CPP
/** The base class for Augmented Lagrangian penalty of state-input constraint. */
class StateInputAugmentedLagrangianInterface {
 public:
  StateInputAugmentedLagrangianInterface() = default;
  virtual ~StateInputAugmentedLagrangianInterface() = default;
  virtual StateInputAugmentedLagrangianInterface* clone() const = 0;

  /** Check penalty's activity */
  virtual bool isActive(scalar_t time) const = 0;

  /** Get the size of the constraint vector at given time */
  virtual size_t getNumConstraints(scalar_t time) const = 0;

  /** Get the constraint and its penalty value */
  virtual LagrangianMetrics getValue(scalar_t time, const vector_t& state, const vector_t& input, const Multiplier& lagrangian,
                                     const PreComputation& preComp) const = 0;

  /** Get the constraint's penalty quadratic approximation */
  virtual ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                         const Multiplier& lagrangian,
                                                                         const PreComputation& preComp) const = 0;

  /** Update Lagrange/penalty multipliers and the penalty function value. */
  virtual std::pair<Multiplier, scalar_t> updateLagrangian(scalar_t time, const vector_t& state, const vector_t& input,
                                                           const vector_t& constraint, const Multiplier& lagrangian) const = 0;

  /** Initialize Lagrange/penalty multipliers. */
  virtual Multiplier initializeLagrangian(scalar_t time) const = 0;

 protected:
  StateInputAugmentedLagrangianInterface(const StateInputAugmentedLagrangianInterface& rhs) = default;
};
```

状态-输入拉格朗日基类.

### StateInputAugmentedLagrangian类

```CPP
/** The base class for Augmented Lagrangian penalty of state-input constraint. */
class StateInputAugmentedLagrangian final : public StateInputAugmentedLagrangianInterface {
 public:
  /**
   * Constructor.
   * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
   * @param [in] penaltyPtrArray: An array of pointers to the penalty function on the constraint.
   */
  StateInputAugmentedLagrangian(std::unique_ptr<StateInputConstraint> constraintPtr,
                                std::vector<std::unique_ptr<augmented::AugmentedPenaltyBase>> penaltyPtrArray);

  /**
   * Constructor.
   * @note This allows a varying number of constraints and uses the same penalty function for each constraint.
   * @param [in] constraintPtr: A pointer to the constraint which will be enforced as soft constraints.
   * @param [in] penaltyPtr: A pointer to the penalty function on the constraint.
   */
  StateInputAugmentedLagrangian(std::unique_ptr<StateInputConstraint> constraintPtr,
                                std::unique_ptr<augmented::AugmentedPenaltyBase> penaltyPtr);

  StateInputAugmentedLagrangian* clone() const override;
  bool isActive(scalar_t time) const override;
  size_t getNumConstraints(scalar_t time) const override;

  LagrangianMetrics getValue(scalar_t time, const vector_t& state, const vector_t& input, const Multiplier& multiplier,
                             const PreComputation& preComp) const override;

  ScalarFunctionQuadraticApproximation getQuadraticApproximation(scalar_t time, const vector_t& state, const vector_t& input,
                                                                 const Multiplier& multiplier,
                                                                 const PreComputation& preComp) const override;

  std::pair<Multiplier, scalar_t> updateLagrangian(scalar_t time, const vector_t& /*state*/, const vector_t& /*input*/,
                                                   const vector_t& constraint, const Multiplier& multiplier) const override;

  Multiplier initializeLagrangian(scalar_t time) const override;

  /** Gets the wrapped constraint. */
  template <typename Derived = StateInputConstraint>
  Derived& get() {
    static_assert(std::is_base_of<StateInputConstraint, Derived>::value, "Template argument must derive from StateInputConstraint!");
    return dynamic_cast<Derived&>(*constraintPtr_);
  }

 private:
  StateInputAugmentedLagrangian(const StateInputAugmentedLagrangian& other);

  std::unique_ptr<StateInputConstraint> constraintPtr_;
  MultidimensionalPenalty penalty_;
};
```

状态-输入拉格朗日类.

## 推导状态Rollout

`Rollout`表示根据当前输入与初始状态，使用积分算法推导动态系统指定时间范围内的状态.

### Rollout Settings

```CPP
/**
 * This structure contains the settings for forward rollout algorithms.
 */
struct Settings {
  /** This value determines the absolute tolerance error for ode solvers. */
  scalar_t absTolODE = 1e-9;
  /** This value determines the relative tolerance error for ode solvers. */
  scalar_t relTolODE = 1e-6;
  /** This value determines the maximum number of integration points per a second for ode solvers. */
  size_t maxNumStepsPerSecond = 10000;
  /** The integration time step used in the fixed time-step rollout methods */
  scalar_t timeStep = 1e-2;
  /** Rollout integration scheme type */
  IntegratorType integratorType = IntegratorType::ODE45;

  /** Whether to check that the rollout is numerically stable */
  bool checkNumericalStability = false;
  /** Whether to run controller again after integration to construct input trajectory */
  bool reconstructInputTrajectory = true;

  /** Which of the RootFinding algorithms to use in StateRollout
   *   0:  Anderson & Björck  (default)
   *   1:  Pegasus
   *   2:  Illinois
   *   3:  Regula Falsi
   */
  RootFinderType rootFindingAlgorithm = RootFinderType::ANDERSON_BJORCK;
  /** This value determines the maximum number of iterations, per event, allowed in state triggered rollout to find
   *  the guard surface zero crossing.  */
  int maxSingleEventIterations = 10;
  /** Whether to use the trajectory spreading controller in state triggered rollout */
  bool useTrajectorySpreadingController = false;
};
```

表示`ode`求解器的步长与容限.

### RolloutBase

```CPP
/**
 * This class is an interface class for forward rollout of the system dynamics.
 */
class RolloutBase {
 public:
  /**
   * Default constructor.
   *
   * @param [in] rolloutSettings: The rollout settings.
   */
  explicit RolloutBase(rollout::Settings rolloutSettings) : rolloutSettings_(std::move(rolloutSettings)) {}

  /**
   * Default destructor.
   */
  virtual ~RolloutBase() = default;

  /**
   * Returns the rollout settings.
   *
   * @return The rollout settings.
   */
  const rollout::Settings& settings() const { return rolloutSettings_; }

  /**
   * The kills the integrator inside the rollout.
   */
  virtual void abortRollout() {}

  /**
   * The enables the integrator inside the rollout to start again.
   */
  virtual void reactivateRollout() {}

  /**
   * Resets the simulator state to the initial state in the next runImpl.
   * @note This is relevant if a physics engine (e.g. RaiSim) is used.
   */
  virtual void resetRollout() {}

  /**
   * Returns pointer to the class.
   *
   * @return A raw pointer to the class.
   */
  virtual RolloutBase* clone() const = 0;

  /**
   * Forward integrate the system dynamics with given controller. It uses the given control policies and initial state,
   * to integrate the system dynamics in time period [initTime, finalTime].
   *
   * @param [in] initTime: The initial time.
   * @param [in] initState: The initial state.
   * @param [in] finalTime: The final time.
   * @param [in] controller: control policy.
   * @param [in, out] modeSchedule: Defines the sequence of modes and the associated event times. For TimeTriggeredRollout
   *                                this is an input argument while for StateTriggeredRollout this is an output argument.
   * @param [out] timeTrajectory: The time trajectory stamp.
   * @param [out] postEventIndices: Indices containing past-the-end index of events trigger.
   * @param [out] stateTrajectory: The state trajectory.
   * @param [out] inputTrajectory: The control input trajectory.
   *
   * @return The final state (state jump is considered if it took place)
   */
  virtual vector_t run(scalar_t initTime, const vector_t& initState, scalar_t finalTime, ControllerBase* controller,
                       ModeSchedule& modeSchedule, scalar_array_t& timeTrajectory, size_array_t& postEventIndices,
                       vector_array_t& stateTrajectory, vector_array_t& inputTrajectory) = 0;

  /**
   * Prints out the rollout.
   *
   * @param [in] timeTrajectory: The time trajectory stamp.
   * @param [in] postEventIndices: An array of the post-event indices.
   * @param [in] stateTrajectory: The state trajectory.
   * @param [in] inputTrajectory: The control input trajectory.
   */
  static void display(const scalar_array_t& timeTrajectory, const size_array_t& postEventIndices, const vector_array_t& stateTrajectory,
                      const vector_array_t* const inputTrajectory);

 protected:
  /** Extracts an array of the rollout's start and final times for each active mode. */
  std::vector<std::pair<scalar_t, scalar_t>> findActiveModesTimeInterval(scalar_t initTime, scalar_t finalTime,
                                                                         const scalar_array_t& eventTimes) const;

  /** Checks for the numerical stability if rollout::Settings::checkNumericalStability is true. */
  void checkNumericalStability(const ControllerBase& controller, const scalar_array_t& timeTrajectory, const size_array_t& postEventIndices,
                               const vector_array_t& stateTrajectory, const vector_array_t& inputTrajectory) const;

  const rollout::Settings rolloutSettings_;
};
```

`Rollout`的基类,继承这个类并实现虚函数可以使用它来推导状态.

```CPP
/**
  * Forward integrate the system dynamics with given controller. It uses the given control policies and initial state,
  * to integrate the system dynamics in time period [initTime, finalTime].
  *
  * @param [in] initTime: The initial time.
  * @param [in] initState: The initial state.
  * @param [in] finalTime: The final time.
  * @param [in] controller: control policy.
  * @param [in, out] modeSchedule: Defines the sequence of modes and the associated event times. For TimeTriggeredRollout
  *                                this is an input argument while for StateTriggeredRollout this is an output argument.
  * @param [out] timeTrajectory: The time trajectory stamp.
  * @param [out] postEventIndices: Indices containing past-the-end index of events trigger.
  * @param [out] stateTrajectory: The state trajectory.
  * @param [out] inputTrajectory: The control input trajectory.
  *
  * @return The final state (state jump is considered if it took place)
  */
virtual vector_t run(scalar_t initTime, const vector_t& initState, scalar_t finalTime, ControllerBase* controller,
                      ModeSchedule& modeSchedule, scalar_array_t& timeTrajectory, size_array_t& postEventIndices,
                      vector_array_t& stateTrajectory, vector_array_t& inputTrajectory) = 0;
```

`run`函数就是会积分系统动态，得出`timeTrajectory`,`postEventIndices`,`stateTrajectory`,`inputTrajectory`,`modeSchedule`.

### TimeTriggeredRollout

```CPP
/**
 * This class is an interface class for forward rollout of the system dynamics.
 */
class TimeTriggeredRollout : public RolloutBase {
 public:
  /**
   * Constructor.
   *
   * @param [in] systemDynamics: The system dynamics for forward rollout.
   * @param [in] rolloutSettings: The rollout settings.
   */
  explicit TimeTriggeredRollout(const ControlledSystemBase& systemDynamics, rollout::Settings rolloutSettings = rollout::Settings());

  ~TimeTriggeredRollout() override = default;
  TimeTriggeredRollout(const TimeTriggeredRollout&) = delete;
  TimeTriggeredRollout& operator=(const TimeTriggeredRollout&) = delete;
  TimeTriggeredRollout* clone() const override { return new TimeTriggeredRollout(*systemDynamicsPtr_, this->settings()); }

  /** Returns the underlying dynamics. */
  ControlledSystemBase* systemDynamicsPtr() { return systemDynamicsPtr_.get(); }

  void abortRollout() override { systemEventHandlersPtr_->killIntegration_ = true; }
  void reactivateRollout() override { systemEventHandlersPtr_->killIntegration_ = false; }

  vector_t run(scalar_t initTime, const vector_t& initState, scalar_t finalTime, ControllerBase* controller, ModeSchedule& modeSchedule,
               scalar_array_t& timeTrajectory, size_array_t& postEventIndices, vector_array_t& stateTrajectory,
               vector_array_t& inputTrajectory) override;

 private:
  std::unique_ptr<PreComputation> preCompPtr_;
  std::unique_ptr<ControlledSystemBase> systemDynamicsPtr_;

  std::shared_ptr<SystemEventHandler> systemEventHandlersPtr_;

  std::unique_ptr<IntegratorBase> dynamicsIntegratorPtr_;
};
```

给定起始时间，初始状态，系统输入，进行积分。

#### run

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
vector_t TimeTriggeredRollout::run(scalar_t initTime, const vector_t& initState, scalar_t finalTime, ControllerBase* controller,
                                   ModeSchedule& modeSchedule, scalar_array_t& timeTrajectory, size_array_t& postEventIndices,
                                   vector_array_t& stateTrajectory, vector_array_t& inputTrajectory) {
  if (initTime > finalTime) {
    throw std::runtime_error("[TimeTriggeredRollout::run] The initial time should be less-equal to the final time!");
  }
  if (controller == nullptr) {
    throw std::runtime_error("[TimeTriggeredRollout::run] Controller is not set!");
  }

  // extract sub-systems
  const auto timeIntervalArray = findActiveModesTimeInterval(initTime, finalTime, modeSchedule.eventTimes);
  const int numSubsystems = timeIntervalArray.size();
  const int numEvents = numSubsystems - 1;

  // max number of steps for integration
  const auto maxNumSteps = static_cast<size_t>(this->settings().maxNumStepsPerSecond * std::max(1.0, finalTime - initTime));

  // clearing the output trajectories
  timeTrajectory.clear();
  timeTrajectory.reserve(maxNumSteps + 1);
  stateTrajectory.clear();
  stateTrajectory.reserve(maxNumSteps + 1);
  inputTrajectory.clear();
  inputTrajectory.reserve(maxNumSteps + 1);
  postEventIndices.clear();
  postEventIndices.reserve(numEvents);

  // set controller
  systemDynamicsPtr_->setController(controller);

  // reset function calls counter
  systemDynamicsPtr_->resetNumFunctionCalls();

  // reset the event class
  systemEventHandlersPtr_->reset();

  vector_t beginState = initState;
  int k_u = 0;  // control input iterator
  for (int i = 0; i < numSubsystems; i++) {
    if (timeIntervalArray[i].first < timeIntervalArray[i].second) {
      Observer observer(&stateTrajectory, &timeTrajectory);  // concatenate trajectory
      // integrate controlled system
      dynamicsIntegratorPtr_->integrateAdaptive(*systemDynamicsPtr_, observer, beginState, timeIntervalArray[i].first,
                                                timeIntervalArray[i].second, this->settings().timeStep, this->settings().absTolODE,
                                                this->settings().relTolODE, maxNumSteps);
    } else {
      timeTrajectory.push_back(timeIntervalArray[i].second);
      stateTrajectory.push_back(beginState);
    }

    // compute control input trajectory and concatenate to inputTrajectory
    if (this->settings().reconstructInputTrajectory) {
      for (; k_u < timeTrajectory.size(); k_u++) {
        inputTrajectory.emplace_back(systemDynamicsPtr_->controllerPtr()->computeInput(timeTrajectory[k_u], stateTrajectory[k_u]));
      }  // end of k_u loop
    }

    // a jump has taken place
    if (i < numEvents) {
      postEventIndices.push_back(stateTrajectory.size());
      // jump map
      beginState = systemDynamicsPtr_->computeJumpMap(timeTrajectory.back(), stateTrajectory.back());
    }
  }  // end of i loop

  // check for the numerical stability
  this->checkNumericalStability(*controller, timeTrajectory, postEventIndices, stateTrajectory, inputTrajectory);

  return stateTrajectory.back();
}
```

实际进行积分，使用积分器`dynamicsIntegratorPtr_`的`integrateAdaptive`函数进行自适应积分，使用`Observer`存储积分结果.

## Integrator

这是`ocs2`的积分器，它使用不同的积分方法，对系统动态进行积分.

### IntegratorBase

```CPP
/**
 * The interface class for integration of autonomous systems.
 */
class IntegratorBase {
 public:
  using system_func_t = std::function<void(const vector_t& x, vector_t& dxdt, scalar_t t)>;
  using observer_func_t = std::function<void(const vector_t& x, scalar_t t)>;

  /**
   * Default constructor
   * @param [in] eventHandler
   */
  explicit IntegratorBase(std::shared_ptr<SystemEventHandler> eventHandlerPtr = nullptr);

  /**
   * Default destructor
   */
  virtual ~IntegratorBase() = default;

  /**
   * Equidistant integration based on initial and final time as well as step length.
   *
   * @param [in] system: System dynamics
   * @param [in] observer: Observer
   * @param [in] initialState: Initial state.
   * @param [in] startTime: Initial time.
   * @param [in] finalTime: Final time.
   * @param [in] dt: Time step.
   */
  void integrateConst(OdeBase& system, Observer& observer, const vector_t& initialState, scalar_t startTime, scalar_t finalTime,
                      scalar_t dt, int maxNumSteps = std::numeric_limits<int>::max());

  /**
   * Adaptive time integration based on start time and final time.
   *
   * @param [in] system: System dynamics
   * @param [in] observer: Observer
   * @param [in] initialState: Initial state.
   * @param [in] startTime: Initial time.
   * @param [in] finalTime: Final time.
   * @param [in] dtInitial: Initial time step.
   * @param [in] AbsTol: The absolute tolerance error for ode solver.
   * @param [in] RelTol: The relative tolerance error for ode solver.
   */
  void integrateAdaptive(OdeBase& system, Observer& observer, const vector_t& initialState, scalar_t startTime, scalar_t finalTime,
                         scalar_t dtInitial = 0.01, scalar_t AbsTol = 1e-6, scalar_t RelTol = 1e-3,
                         int maxNumSteps = std::numeric_limits<int>::max());

  /**
   * Output integration based on a given time trajectory.
   *
   * @param [in] system: System dynamics
   * @param [in] observer: Observer
   * @param [in] initialState: Initial state.
   * @param [in] beginTimeItr: The iterator to the beginning of the time stamp trajectory.
   * @param [in] endTimeItr: The iterator to the end of the time stamp trajectory.
   * @param [in] dtInitial: Initial time step.
   * @param [in] AbsTol: The absolute tolerance error for ode solver.
   * @param [in] RelTol: The relative tolerance error for ode solver.
   */
  void integrateTimes(OdeBase& system, Observer& observer, const vector_t& initialState,
                      typename scalar_array_t::const_iterator beginTimeItr, typename scalar_array_t::const_iterator endTimeItr,
                      scalar_t dtInitial = 0.01, scalar_t AbsTol = 1e-6, scalar_t RelTol = 1e-3,
                      int maxNumSteps = std::numeric_limits<int>::max());

 protected:
  /** Copy constructor */
  IntegratorBase(const IntegratorBase& rhs) = default;

  system_func_t systemFunction(OdeBase& system, int maxNumSteps) const;

  virtual void runIntegrateConst(system_func_t system, observer_func_t observer, const vector_t& initialState, scalar_t startTime,
                                 scalar_t finalTime, scalar_t dt) = 0;

  virtual void runIntegrateAdaptive(system_func_t system, observer_func_t observer, const vector_t& initialState, scalar_t startTime,
                                    scalar_t finalTime, scalar_t dtInitial, scalar_t AbsTol, scalar_t RelTol) = 0;

  virtual void runIntegrateTimes(system_func_t system, observer_func_t observer, const vector_t& initialState,
                                 typename scalar_array_t::const_iterator beginTimeItr, typename scalar_array_t::const_iterator endTimeItr,
                                 scalar_t dtInitial, scalar_t AbsTol, scalar_t RelTol) = 0;

 private:
  std::shared_ptr<SystemEventHandler> eventHandlerPtr_;
};
```

`integrateAdaptive`接受系统动态，进行变步长自适应积分.

`integrateConst`接受系统动态，进行定步长积分.

`integrateTimes`接受一系列时间，进行变步长自适应积分。

#### integrateAdaptive

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void IntegratorBase::integrateAdaptive(OdeBase& system, Observer& observer, const vector_t& initialState, scalar_t startTime,
                                       scalar_t finalTime, scalar_t dtInitial /*= 0.01*/, scalar_t AbsTol /*= 1e-6*/,
                                       scalar_t RelTol /*= 1e-3*/, int maxNumSteps /*= std::numeric_limits<int>::max()*/) {
  observer_func_t callback = [&](const vector_t& x, scalar_t t) {
    observer.observe(x, t);
    eventHandlerPtr_->handleEvent(system, t, x);
  };
  runIntegrateAdaptive(systemFunction(system, maxNumSteps), callback, initialState, startTime, finalTime, dtInitial, AbsTol, RelTol);
}
```

运行`runIntegrateAdaptive`

### RungeKuttaDormandPrince5

```CPP
/*
 * 5th order Runge Kutta Dormand-Prince (ode45) Integrator class
 *
 * The implementation is based on the boost odeint integrator with the controlled
 * boost::numeric::odeint::runge_kutta_dopri5 stepper.
 */
class RungeKuttaDormandPrince5 : public IntegratorBase {
 public:
  explicit RungeKuttaDormandPrince5(std::shared_ptr<SystemEventHandler> eventHandlerPtr = nullptr)
      : IntegratorBase(std::move(eventHandlerPtr)){};

  ~RungeKuttaDormandPrince5() override = default;

 private:
  /**
   * Equidistant integration based on initial and final time as well as step length.
   *
   * @param [in] system: System function
   * @param [in] observer: Observer callback
   * @param [in] initialState: Initial state.
   * @param [in] startTime: Initial time.
   * @param [in] finalTime: Final time.
   * @param [in] dt: Time step.
   */
  void runIntegrateConst(system_func_t system, observer_func_t observer, const vector_t& initialState, scalar_t startTime,
                         scalar_t finalTime, scalar_t dt) override;

  /**
   * Adaptive time integration based on start time and final time.
   *
   * @param [in] system: System function
   * @param [in] observer: Observer callback
   * @param [in] initialState: Initial state.
   * @param [in] startTime: Initial time.
   * @param [in] finalTime: Final time.
   * @param [in] dtInitial: Initial time step.
   * @param [in] absTol: The absolute tolerance error for ode solver.
   * @param [in] relTol: The relative tolerance error for ode solver.
   */
  void runIntegrateAdaptive(system_func_t system, observer_func_t observer, const vector_t& initialState, scalar_t startTime,
                            scalar_t finalTime, scalar_t dtInitial, scalar_t absTol, scalar_t relTol) override;
  /**
   * Output integration based on a given time trajectory.
   *
   * @param [in] system: System function
   * @param [in] observer: Observer callback
   * @param [in] initialState: Initial state.
   * @param [in] beginTimeItr: The iterator to the beginning of the time stamp trajectory.
   * @param [in] endTimeItr: The iterator to the end of the time stamp trajectory.
   * @param [in] dtInitial: Initial time step.
   * @param [in] absTol: The absolute tolerance error for ode solver.
   * @param [in] relTol: The relative tolerance error for ode solver.
   */
  void runIntegrateTimes(system_func_t system, observer_func_t observer, const vector_t& initialState,
                         typename scalar_array_t::const_iterator beginTimeItr, typename scalar_array_t::const_iterator endTimeItr,
                         scalar_t dtInitial, scalar_t absTol, scalar_t relTol) override;

  static constexpr size_t maxNumStepsRetries_ = 100;
};
```

著名的ode45方法.

## Controller

`Controller`管理最优化的控制输入，这个控制输入是一个离散的时间-输入序列，`Controller`负责将它推广到任意时间点上.使用线性插值等.

### ControllerBase

```CPP
/**
 * The base class for all controllers.
 */
class ControllerBase {
 public:
  /** Constructor */
  ControllerBase() = default;

  /** Default destructor. */
  virtual ~ControllerBase() = default;

  /**
   * @brief Computes the control command at a given time and state.
   *
   * @param [in] t: Current time.
   * @param [in] x: Current state.
   * @return Current input.
   */
  virtual vector_t computeInput(scalar_t t, const vector_t& x) = 0;

  /**
   * @brief Merges this controller with another controller that comes active later in time
   * This method is typically used to merge controllers from multiple time partitions.
   * Indices specifying a range of elements. Copies of the elements in the range [index, index_length) are inserted at the end.
   *
   * @note Only controllers of the same type can be merged.
   *
   * @param[in] otherController: The control law to be appended.
   * @param[in] index: The starting index
   * @param[in] length: The length of the copy.
   */
  virtual void concatenate(const ControllerBase* otherController, int index, int length) = 0;

  /**
   * @brief Merges this controller with another controller that comes active later in time
   * This method is typically used to merge controllers from multiple time partitions.
   * @note Only controllers of the same type can be merged.
   *
   * @param[in] otherController: The control law to be appended.
   */
  void concatenate(const ControllerBase* otherController) { concatenate(otherController, 0, otherController->size()); }

  /**
   * @brief Returns the size of the controller.
   *
   * @return The size of the controller.
   */
  virtual int size() const = 0;

  /**
   * @brief Prints the type of controller
   * @return ControllerType: what type of controller this is
   */
  virtual ControllerType getType() const = 0;

  /**
   * @brief clears and reverts back to an empty controller.
   * Therefore, if empty() method is called, it will return true.
   */
  virtual void clear() = 0;

  /**
   * Returns whether the class contains any information.
   *
   * @return true if it contains no information, false otherwise.
   */
  virtual bool empty() const = 0;

  /**
   * @brief Create a deep copy of the object.
   * @warning Cloning implies that the caller takes ownership and deletes the created object.
   * @return Pointer to a new instance.
   */
  virtual ControllerBase* clone() const = 0;

  /**
   * Displays controller's data.
   */
  virtual void display() const {}

  /**
   * @brief Gets the event times for which the controller is designed.
   * @return The event times of the controller.
   */
  virtual scalar_array_t controllerEventTimes() const { return {}; }

  /**
   * Saves the controller at given time to an array of arrays structure for ROS transmission
   *
   * @param[in] timeArray array of query times
   * @param[out] flatArray2 The array of arrays that is to be filled, i.e., the compressed controller. One array per query time
   */
  virtual void flatten(const scalar_array_t& timeArray, const std::vector<std::vector<float>*>& flatArray2) const {
    throw std::runtime_error("ControllerBase::flatten: not implemented.");
  }

 protected:
  /** Copy constructor */
  ControllerBase(const ControllerBase& rhs) = default;
};
```

这是控制器通用基类.通过实现虚函数来实现控制器.

#### computeInput成员函数

```CPP
/**
 * @brief Computes the control command at a given time and state.
  *
  * @param [in] t: Current time.
  * @param [in] x: Current state.
  * @return Current input.
  */
virtual vector_t computeInput(scalar_t t, const vector_t& x) = 0;
```

`computeInput`计算当前时间点$t$与当前状态$\textbf{x}$系统的控制输入$\textbf{u}$

#### concatenate

```CPP
/**
  * @brief Merges this controller with another controller that comes active later in time
  * This method is typically used to merge controllers from multiple time partitions.
  * Indices specifying a range of elements. Copies of the elements in the range [index, index_length) are inserted at the end.
  *
  * @note Only controllers of the same type can be merged.
  *
  * @param[in] otherController: The control law to be appended.
  * @param[in] index: The starting index
  * @param[in] length: The length of the copy.
  */
virtual void concatenate(const ControllerBase* otherController, int index, int length) = 0;

/**
  * @brief Merges this controller with another controller that comes active later in time
  * This method is typically used to merge controllers from multiple time partitions.
  * @note Only controllers of the same type can be merged.
  *
  * @param[in] otherController: The control law to be appended.
  */
void concatenate(const ControllerBase* otherController) { concatenate(otherController, 0, otherController->size()); }
```

`concatenate`函数将另一个控制器连接到这个控制器后.

### FeedforwardController类

`FeedforwardController`提供是一个简单的控制器.进行简单的线性插值.

```CPP
/**
 * FeedforwardController provides a time-dependent control law without state-dependent feedback.
 * Commonly, this is used to wrap around a more general controller and extract only the feedforward portion.
 */
class FeedforwardController final : public ControllerBase {
 public:
  /** Constructor, leaves object uninitialized */
  FeedforwardController() = default;

  /**
   * Constructor initializes all required members of the controller.
   *
   * @param [in] controllerTime: Time stamp array of the controller
   * @param [in] controllerFeedforward: The feedforward control input array.
   */
  FeedforwardController(scalar_array_t controllerTime, vector_array_t controllerFeedforward)
      : timeStamp_(std::move(controllerTime)), uffArray_(std::move(controllerFeedforward)) {}

  /**
   * Constructor to initialize the feedforward input data with a general controller rolled-out along a nominal stateTrajectory
   *
   * @param [in] controllerTime the times for the rollout
   * @param [in] stateTrajectory the states for the rollout
   * @param [in] controller the controller to extract the feedforward controls from during a rollout
   */
  FeedforwardController(const scalar_array_t& controllerTime, const vector_array_t& stateTrajectory, ControllerBase* controller);

  /** Copy constructor */
  FeedforwardController(const FeedforwardController& other);

  /** Move constructor */
  FeedforwardController(FeedforwardController&& other);

  /** Copy assignment (copy and swap idiom) */
  FeedforwardController& operator=(FeedforwardController rhs);

  /** Destructor */
  ~FeedforwardController() override = default;

  /**
   * setController Assign control law
   * @param [in] controllerTime: Time stamp array of the controller
   * @param [in] controllerFeedforward: The feedforward control input array.
   */
  void setController(const scalar_array_t& controllerTime, const vector_array_t& controllerFeedforward);

  vector_t computeInput(scalar_t t, const vector_t& x) override;

  void concatenate(const ControllerBase* nextController, int index, int length) override;

  int size() const override;

  ControllerType getType() const override;

  void clear() override;

  bool empty() const override;

  FeedforwardController* clone() const override;

  void display() const override;

  void flatten(const scalar_array_t& timeArray, const std::vector<std::vector<float>*>& flatArray2) const override;

  static FeedforwardController unFlatten(const scalar_array_t& timeArray, const std::vector<std::vector<float> const*>& flatArray2);

 private:
  void flattenSingle(scalar_t time, std::vector<float>& flatArray) const;

 public:
  scalar_array_t timeStamp_;
  vector_array_t uffArray_;

  friend void swap(FeedforwardController& a, FeedforwardController& b) noexcept;
};
```

#### setController

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void FeedforwardController::setController(const scalar_array_t& controllerTime, const vector_array_t& controllerFeedforward) {
  timeStamp_ = controllerTime;
  uffArray_ = controllerFeedforward;
}
```

#### computeInput

```CPP
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
vector_t FeedforwardController::computeInput(scalar_t t, const vector_t& x) {
  return LinearInterpolation::interpolate(t, timeStamp_, uffArray_);
}
```

进行简单的线性插值，获得最优控制解.

#### concatenate

```CPP
******************************************************************************************************/
/******************************************************************************************************/
void FeedforwardController::concatenate(const ControllerBase* nextController, int index, int length) {
  if (auto nextFfwdCtrl = dynamic_cast<const FeedforwardController*>(nextController)) {
    if (!timeStamp_.empty() && timeStamp_.back() > nextFfwdCtrl->timeStamp_.front()) {
      throw std::runtime_error("Concatenate requires that the nextController comes later in time.");
    }
    int last = index + length;
    timeStamp_.insert(timeStamp_.end(), nextFfwdCtrl->timeStamp_.begin() + index, nextFfwdCtrl->timeStamp_.begin() + last);
    uffArray_.insert(uffArray_.end(), nextFfwdCtrl->uffArray_.begin() + index, nextFfwdCtrl->uffArray_.begin() + last);
  } else {
    throw std::runtime_error("Concatenate only works with controllers of the same type.");
  }
}
```

将控制器延长.

### LinearController

```CPP

/**
 * LinearController implements a time and state dependent controller of the
 * form u[x,t] = k[t] * x + uff[t]
 */
class LinearController final : public ControllerBase {
 public:
  /** Constructor, leaves object uninitialized */
  LinearController() = default;

  /**
   * @brief Constructor initializes all required members of the controller.
   *
   * @param [in] controllerTime: Time stamp array of the controller
   * @param [in] controllerBias: The bias array.
   * @param [in] controllerGain: The feedback gain array.
   */
  LinearController(scalar_array_t controllerTime, vector_array_t controllerBias, matrix_array_t controllerGain)
      : timeStamp_(std::move(controllerTime)), biasArray_(std::move(controllerBias)), gainArray_(std::move(controllerGain)) {}

  /** Copy constructor */
  LinearController(const LinearController& other);

  /** Move constructor */
  LinearController(LinearController&& other);

  /** Copy assignment (copy and swap idiom) */
  LinearController& operator=(LinearController rhs);

  /** Destructor */
  ~LinearController() override = default;

  /** Clone */
  LinearController* clone() const override;

  /**
   * @brief setController Assign control law
   * @param [in] controllerTime: Time stamp array of the controller
   * @param [in] controllerBias: The bias array.
   * @param [in] controllerGain: The feedback gain array.
   */
  void setController(const scalar_array_t& controllerTime, const vector_array_t& controllerBias, const matrix_array_t& controllerGain);

  vector_t computeInput(scalar_t t, const vector_t& x) override;

  void concatenate(const ControllerBase* nextController, int index, int length) override;

  int size() const override;

  ControllerType getType() const override;

  void clear() override;

  bool empty() const override;

  void display() const override;

  void getFeedbackGain(scalar_t time, matrix_t& gain) const;

  void getBias(scalar_t time, vector_t& bias) const;

  scalar_array_t controllerEventTimes() const override;

  void flatten(const scalar_array_t& timeArray, const std::vector<std::vector<float>*>& flatArray2) const override;

  static LinearController unFlatten(const size_array_t& stateDim, const size_array_t& inputDim, const scalar_array_t& timeArray,
                                    const std::vector<std::vector<float> const*>& flatArray2);

 private:
  void flattenSingle(scalar_t time, std::vector<float>& flatArray) const;

 public:
  scalar_array_t timeStamp_;
  vector_array_t biasArray_;
  vector_array_t deltaBiasArray_;
  matrix_array_t gainArray_;

  friend void swap(LinearController& a, LinearController& b) noexcept;
};
```

`LinearController`实现的是有状态反馈的控制器.

$$
\textbf{u}(\textbf{x},t) = k[t]\textbf{x} + u_{ff}[t]
$$

### setController

```CPP
void LinearController::setController(const scalar_array_t& controllerTime, const vector_array_t& controllerBias,
                                     const matrix_array_t& controllerGain) {
  timeStamp_ = controllerTime;
  biasArray_ = controllerBias;
  gainArray_ = controllerGain;
}
```

### computeInput

```CPP
vector_t LinearController::computeInput(scalar_t t, const vector_t& x) {
  const auto indexAlpha = LinearInterpolation::timeSegment(t, timeStamp_);

  vector_t uff = LinearInterpolation::interpolate(indexAlpha, biasArray_);
  const matrix_t k = LinearInterpolation::interpolate(indexAlpha, gainArray_);

  uff.noalias() += k * x;
  return uff;
}
```

## 罚函数Penalties

罚函数是优化问题中的对于超过约束的惩罚函数，用于将不等式约束或软约束添加到代价函数中去.罚函数假定为凸的.是时间与约束违反(就是约束函数的值)的函数.

### PenaltyBase

```CPP
class PenaltyBase {
 public:
  /** Default constructor */
  PenaltyBase() = default;

  /** Default destructor */
  virtual ~PenaltyBase() = default;

  /** Clones the class */
  virtual PenaltyBase* clone() const = 0;

  /** Get the name of the penalty function. This method is only used during error handling. */
  virtual std::string name() const = 0;

  /**
   * Compute the penalty value at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: Constraint value.
   * @return penalty cost.
   */
  virtual scalar_t getValue(scalar_t t, scalar_t h) const = 0;

  /**
   * Compute the penalty derivative at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: Constraint value.
   * @return penalty derivative with respect to constraint value.
   */
  virtual scalar_t getDerivative(scalar_t t, scalar_t h) const = 0;

  /**
   * Compute the penalty second derivative at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] h: Constraint value.
   * @return penalty second derivative with respect to constraint value.
   */
  virtual scalar_t getSecondDerivative(scalar_t t, scalar_t h) const = 0;

 protected:
  PenaltyBase(const PenaltyBase& other) = default;
};
```

罚函数基类,实现纯虚函数就可以.

### DoubleSidedPenalty

```CPP

/**
 * Implements the double sided inequality \f$ l \leq h \leq u \f$ with a given penalty function \f$ p() \f$.
 *
 * \f[
 *   p_{box}(h) = p(h - l) + p(u - h)
 * \f]
 */
class DoubleSidedPenalty final : public PenaltyBase {
 public:
  /**
   * Constructor
   * @param [in] lowerBound: The lower bound.
   * @param [in] upperBound: The upper bound.
   * @param [in] penalty: The penalty for the two inequality constraint.
   */
  DoubleSidedPenalty(scalar_t lowerBound, scalar_t upperBound, std::unique_ptr<PenaltyBase> penalty)
      : lowerBound_(lowerBound), upperBound_(upperBound), penaltyPtr_(std::move(penalty)) {}

  ~DoubleSidedPenalty() override = default;
  DoubleSidedPenalty* clone() const override { return new DoubleSidedPenalty(*this); }
  std::string name() const override { return "DoubleSidedPenalty"; }

  scalar_t getValue(scalar_t t, scalar_t h) const override {
    return penaltyPtr_->getValue(t, h - lowerBound_) + penaltyPtr_->getValue(t, upperBound_ - h);
  }
  scalar_t getDerivative(scalar_t t, scalar_t h) const override {
    return penaltyPtr_->getDerivative(t, h - lowerBound_) - penaltyPtr_->getDerivative(t, upperBound_ - h);
  }
  scalar_t getSecondDerivative(scalar_t t, scalar_t h) const override {
    return penaltyPtr_->getSecondDerivative(t, h - lowerBound_) + penaltyPtr_->getSecondDerivative(t, upperBound_ - h);
  }

 private:
  DoubleSidedPenalty(const DoubleSidedPenalty& other)
      : lowerBound_(other.lowerBound_), upperBound_(other.upperBound_), penaltyPtr_(other.penaltyPtr_->clone()) {}

  const scalar_t lowerBound_;
  const scalar_t upperBound_;
  std::unique_ptr<PenaltyBase> penaltyPtr_;
};
```

实现了一个双向的不等式

$$
l \leq h \leq u
$$

的罚函数.

$$
p_{box}(h) = p(h - l) + p(u - h)
$$

其中$p$是单向不等式的罚函数.

### QuadraticPenalty

```CPP
class QuadraticPenalty final : public PenaltyBase {
 public:
  /**
   * Thos constructor sets both the scale and stepLength the same. This is a common practice in Augmented Lagrangian.
   * @param [in] scale: Scaling of the cost.
   */
  explicit QuadraticPenalty(scalar_t scale) : scale_(scale) {}

  ~QuadraticPenalty() override = default;
  QuadraticPenalty* clone() const override { return new QuadraticPenalty(*this); }
  std::string name() const override { return "QuadraticPenalty"; }

  scalar_t getValue(scalar_t t, scalar_t h) const override { return 0.5 * scale_ * h * h; }
  scalar_t getDerivative(scalar_t t, scalar_t h) const override { return scale_ * h; }
  scalar_t getSecondDerivative(scalar_t t, scalar_t h) const override { return scale_; }

 private:
  QuadraticPenalty(const QuadraticPenalty& other) = default;

  const scalar_t scale_;
};
```

实现了等式软约束

$$
h = 0
$$

的二次型罚函数.

$$
L_{A} = \frac{\mu}{2} h^2
$$

其中$\mu$是一个系数.

### RelaxedBarrierPenalty

```CPP
/**
 * Implements the relaxed barrier function for a single inequality constraint \f$ h \geq 0 \f$
 *
 * \f[
 *   p(h)=\left\lbrace
 *               \begin{array}{ll}
 *                 -\mu \ln(h) & if \quad  h > \delta, \\
 *                 -\mu \ln(\delta) + \mu \frac{1}{2} \left( \left( \frac{h-2\delta}{\delta} \right)^2 - 1 \right) & otherwise,
 *               \end{array}
 *             \right.
 * \f]
 *
 * where \f$ \mu \geq 0 \f$, and \f$ \delta \geq 0 \f$ are user defined parameters.
 */
class RelaxedBarrierPenalty final : public PenaltyBase {
 public:
  /**
   * Configuration object for the relaxed barrier penalty.
   * mu : scaling factor
   * delta: relaxation parameter, see class description
   */
  struct Config {
    Config() : Config(1.0, 1e-3) {}
    Config(scalar_t muParam, scalar_t deltaParam) : mu(muParam), delta(deltaParam) {}
    scalar_t mu;
    scalar_t delta;
  };

  /**
   * Constructor
   * @param [in] config: Configuration object containing mu and delta.
   */
  explicit RelaxedBarrierPenalty(Config config) : config_(std::move(config)) {}

  ~RelaxedBarrierPenalty() override = default;
  RelaxedBarrierPenalty* clone() const override { return new RelaxedBarrierPenalty(*this); }
  std::string name() const override { return "RelaxedBarrierPenalty"; }

  scalar_t getValue(scalar_t t, scalar_t h) const override;
  scalar_t getDerivative(scalar_t t, scalar_t h) const override;
  scalar_t getSecondDerivative(scalar_t t, scalar_t h) const override;

 private:
  RelaxedBarrierPenalty(const RelaxedBarrierPenalty& other) = default;

  const Config config_;
};
```

实现了经典的松弛障碍法罚函数.

是不等式约束

$$
h \geq 0
$$

的松弛障碍函数

$$
 p(h)=\left\lbrace
             \begin{array}{ll}
                 -\mu \ln(h) & if \quad  h > \delta, \\
                -\mu \ln(\delta) + \mu \frac{1}{2} \left( \left( \frac{h-2\delta}{\delta} \right)^2 - 1 \right) & otherwise,
               \end{array}
      \right.
$$

松弛障碍法相比于障碍法，优势在于迭代初期可以允许迭代点超出可行域,快速逼近最优解，减少迭代次数.​​平衡效率与可行性

#### Config

```CPP
/**
  * Configuration object for the relaxed barrier penalty.
  * mu : scaling factor
  * delta: relaxation parameter, see class description
  */
struct Config {
  Config() : Config(1.0, 1e-3) {}
  Config(scalar_t muParam, scalar_t deltaParam) : mu(muParam), delta(deltaParam) {}
  scalar_t mu;
  scalar_t delta;
};
```

松弛障碍法参数.

#### getValue

```CPP
scalar_t RelaxedBarrierPenalty::getValue(scalar_t t, scalar_t h) const {
  if (h > config_.delta) {
    return -config_.mu * log(h);
  } else {
    const scalar_t delta_h = (h - 2.0 * config_.delta) / config_.delta;
    return config_.mu * (-log(config_.delta) + 0.5 * delta_h * delta_h - 0.5);
  };
}
```

获得当前值

#### getDerivative

```CPP
scalar_t RelaxedBarrierPenalty::getDerivative(scalar_t t, scalar_t h) const {
  if (h > config_.delta) {
    return -config_.mu / h;
  } else {
    return config_.mu * ((h - 2.0 * config_.delta) / (config_.delta * config_.delta));
  };
}
```

获得导数值

#### getSecondDerivative

```CPP
scalar_t RelaxedBarrierPenalty::getSecondDerivative(scalar_t t, scalar_t h) const {
  if (h > config_.delta) {
    return config_.mu / (h * h);
  } else {
    return config_.mu / (config_.delta * config_.delta);
  };
}
```

获得二阶导数值.

### SmoothAbsolutePenalty

```CPP
/**
 * Implements the smooth-absolute function for a single equality constraint \f$ h = 0 \f$
 *
 * \f[
 *   p(h) = \mu sqrt(h^2 + \delta^2).
 * \f]
 *
 * where \f$ \mu > 0 \f$, and \f$ \delta > 0 \f$ are scale and relaxation parameters respectively. Note that
 * \f$ \delta \f$ defines the error bound between the absolute function and its approximation:
 *
 * \f[
 *   | x - sqrt(h^2 + \delta^2) | \leq \delta, \quad \forall x \in R
 * \f]
 */
class SmoothAbsolutePenalty final : public PenaltyBase {
 public:
  /**
   * Configuration object for the smooth absolute penalty.
   * scale: scaling factor, see class description
   * relaxation: relaxation parameter, see class description
   */
  struct Config {
    Config(scalar_t scaleParam = 100.0, scalar_t relaxationParam = 1e-2) : scale(scaleParam), relaxation(relaxationParam) {}
    scalar_t scale;
    scalar_t relaxation;
  };

  /**
   * Constructor
   * @param [in] config: Configuration object containing mu and delta.
   */
  explicit SmoothAbsolutePenalty(Config config) : config_(std::move(config)) {}

  ~SmoothAbsolutePenalty() override = default;
  SmoothAbsolutePenalty* clone() const override { return new SmoothAbsolutePenalty(*this); }
  std::string name() const override { return "SmoothAbsolutePenalty"; }

  scalar_t getValue(scalar_t t, scalar_t h) const override { return config_.scale * sqrt(h * h + config_.relaxation * config_.relaxation); }
  scalar_t getDerivative(scalar_t t, scalar_t h) const override {
    return config_.scale * h / sqrt(h * h + config_.relaxation * config_.relaxation);
  }
  scalar_t getSecondDerivative(scalar_t t, scalar_t h) const override {
    const scalar_t deltaSquare = config_.relaxation * config_.relaxation;
    return config_.scale * deltaSquare / pow(h * h + deltaSquare, 1.5);
  }

 private:
  SmoothAbsolutePenalty(const SmoothAbsolutePenalty& other) = default;

  const Config config_;
};
```

实现了等式约束

$$
h = 0
$$

的平滑绝对值罚函数

$$
p(h) = \mu \sqrt{h^2 + \delta^2}
$$

其中$\mu > 0$放大倍数,$\delta > 0$是放松系数.

是绝对值函数的平滑近似，$\delta$越小，近似程度越高.

### SquaredHingePenalty

```CPP

/**
 * Implements the squared-hinge function for a single inequality constraint \f$ h \geq 0 \f$
 *
 * \f[
 *   p(h)=\left\lbrace
 *               \begin{array}{ll}
 *                 \frac{\mu}{2} (h - \delta)^2 & if \quad  h < \delta, \\
 *                 0 & otherwise,
 *               \end{array}
 *             \right.
 * \f]
 *
 * where \f$ \mu > 0 \f$, and \f$ \delta \in R \f$ are user defined parameters.
 */
class SquaredHingePenalty final : public PenaltyBase {
 public:
  /**
   * Configuration object for the squared hinge penalty.
   * mu : scaling factor
   * delta: relaxation parameter, see class description
   */
  struct Config {
    Config() : Config(100.0, 1e-1) {}
    Config(scalar_t muParam, scalar_t deltaParam) : mu(muParam), delta(deltaParam) {}
    scalar_t mu;
    scalar_t delta;
  };

  /**
   * Constructor
   * @param [in] config: Configuration object containing mu and delta.
   */
  explicit SquaredHingePenalty(Config config) : config_(std::move(config)) {}

  ~SquaredHingePenalty() override = default;
  SquaredHingePenalty* clone() const override { return new SquaredHingePenalty(*this); }
  std::string name() const override { return "SquaredHingePenalty"; }

  scalar_t getValue(scalar_t t, scalar_t h) const override;
  scalar_t getDerivative(scalar_t t, scalar_t h) const override;
  scalar_t getSecondDerivative(scalar_t t, scalar_t h) const override;

 private:
  SquaredHingePenalty(const SquaredHingePenalty& other) = default;

  Config config_;
};
```

实现了不等式约束

$$
h \geq 0
$$

的平滑平方合页惩罚函数

$$
p(h)=\left\lbrace
                \begin{array}{ll}
                 \frac{\mu}{2} (h - \delta)^2 & if \quad  h < \delta, \\
                  0 & otherwise,
               \end{array}
             \right.
$$

## 增广罚函数AugmentedPenalty

罚函数是优化问题中的对于超过约束的惩罚函数，用于将不等式约束或软约束添加到代价函数中去.罚函数假定为凸的.是时间，约束违反(就是约束函数的值)，拉格朗日乘子的的函数.

### AugmentedPenaltyBase

```CPP
/**
 * The penalty function interface class is used to penalize constraint violation by adding a penalty term to the cost function.
 * We assume that the penalty function is convex. In general, the penalty is a function of time, Lagrange multiplier, and
 * constraint violation.
 */
class AugmentedPenaltyBase {
 public:
  /** Default constructor */
  AugmentedPenaltyBase() = default;

  /** Default destructor */
  virtual ~AugmentedPenaltyBase() = default;

  /** Clones the class */
  virtual AugmentedPenaltyBase* clone() const = 0;

  /** Get the name of the penalty function. This method is only used during error handling. */
  virtual std::string name() const = 0;

  /**
   * Compute the penalty value at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] l: The Lagrange multiplier.
   * @param [in] h: Constraint value.
   * @return penalty cost.
   */
  virtual scalar_t getValue(scalar_t t, scalar_t l, scalar_t h) const = 0;

  /**
   * Compute the penalty derivative at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] l: The Lagrange multiplier.
   * @param [in] h: Constraint value.
   * @return penalty derivative with respect to constraint value.
   */
  virtual scalar_t getDerivative(scalar_t t, scalar_t l, scalar_t h) const = 0;

  /**
   * Compute the penalty second derivative at a certain constraint value.
   *
   * @param [in] t: The time that the constraint is evaluated.
   * @param [in] l: The Lagrange multiplier.
   * @param [in] h: Constraint value.
   * @return penalty second derivative with respect to constraint value.
   */
  virtual scalar_t getSecondDerivative(scalar_t t, scalar_t l, scalar_t h) const = 0;

  /**
   * Updates the Lagrange multiplier.
   *
   * @param [in] t: The time stamp.
   * @param [in] l: The Lagrange multiplier.
   * @param [in] h: Constraint values.
   * @return updated Lagrange multiplier.
   */
  virtual scalar_t updateMultiplier(scalar_t t, scalar_t l, scalar_t h) const = 0;

  /**
   * Initializes the Lagrange multiplier.
   *
   * @return initial value of the Lagrange multiplier.
   */
  virtual scalar_t initializeMultiplier() const = 0;

 protected:
  AugmentedPenaltyBase(const AugmentedPenaltyBase& other) = default;
};
```

增广罚函数基类.

### QuadraticPenalty

```CPP
/**
 *  Implements the augmented Lagrangian for a single equality constraint \f$ h = 0 \f$. This leads to the following
 *  augmented Lagrangian:
 *
 *  \f[
 *      L_{A} = L * \lambda h + \frac{\mu}{2} h^2.
 *  \f]
 *
 *  where \f$ L \f$ is the Lagrangian and \f$ \mu \f$ is the scale, while the remaining terms form the penalty
 *  function \f$ p(h, \lambda) \f$.
 *
 *  This is then minimized with the solver, while the Lagrange multipliers are updated as:
 * \f[
 *      \lambda^*_{k+1} = \lambda^*_k - \alpha h^*_{k+1}.
 * \f]
 *
 * with \f$ alpha \f$ is the step-length. In Augmented Lagrangian method this is often set to \f$ \mu \f$.
 */
class QuadraticPenalty final : public AugmentedPenaltyBase {
 public:
  /**
   * Configuration object for the quadratic penalty.
   * scale: scaling factor, see class description
   * stepSize: step-length parameter, see class description
   */
  struct Config {
    Config() : Config(100.0, 0.0) {}
    Config(scalar_t scaleParam, scalar_t stepSizeParam) : scale(scaleParam), stepSize(stepSizeParam) {}
    scalar_t scale;
    scalar_t stepSize;
  };

  /** Constructor */
  explicit QuadraticPenalty(Config config) : config_(std::move(config)) {}

  /** Factory function */
  static std::unique_ptr<QuadraticPenalty> create(Config config) { return std::make_unique<QuadraticPenalty>(std::move(config)); }

  ~QuadraticPenalty() override = default;
  QuadraticPenalty* clone() const override { return new QuadraticPenalty(*this); }
  std::string name() const override { return "QuadraticPenalty"; }

  scalar_t getValue(scalar_t t, scalar_t l, scalar_t h) const override { return -l * h + 0.5 * config_.scale * h * h; }
  scalar_t getDerivative(scalar_t t, scalar_t l, scalar_t h) const override { return -l + config_.scale * h; }
  scalar_t getSecondDerivative(scalar_t t, scalar_t l, scalar_t h) const override { return config_.scale; }

  scalar_t updateMultiplier(scalar_t t, scalar_t l, scalar_t h) const override { return l - config_.stepSize * config_.scale * h; }
  scalar_t initializeMultiplier() const override { return 0.0; }

 private:
  QuadraticPenalty(const QuadraticPenalty& other) = default;

  const Config config_;
};
```

对等式约束

$$
h = 0
$$

构建增广拉格朗日函数

$$
L_{A} = L * \lambda h + \frac{\mu}{2} h^2
$$

## 非切换问题

非切换系统问题指的是没有多种状态切换的系统，这种系统较切换系统较为简单，本节以多种实际机器人模型为例子.

### RobotInterface基类

```CPP
/**
 * This class implements an interface class to all the robotic examples.
 *
 * The lifetime of the returned objects is tied to the lifetime of the robot interface.
 * The exposed objects are not thread-safe and should be cloned to get an exclusive copy.
 */
class RobotInterface {
 public:
  /** Constructor */
  RobotInterface() = default;

  /** Destructor */
  virtual ~RobotInterface() = default;

  /**
   * Gets the ReferenceManager.
   * @return a shared pointer to the ReferenceManager.
   */
  virtual std::shared_ptr<ReferenceManagerInterface> getReferenceManagerPtr() const { return nullptr; }

  /**
   * @brief Get the optimal control problem definition
   * @return reference to the problem object
   */
  virtual const OptimalControlProblem& getOptimalControlProblem() const = 0;

  /**
   * @brief getInitializer
   * @return reference to the internal solver initializer
   */
  virtual const Initializer& getInitializer() const = 0;
};
```

`ocs2`给所有的`robotic example`设计了一个基类，可以方便地操作.

### 倒立摆cartpole

* [cartpole.md](./not_switch_problems/cartpole.md)
