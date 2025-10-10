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

### ControlledSystemBase

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

系统动态的基类，

纯虚函数`computeFlowMap`定义了系统动态

$$
\dot{\mathbf x}(t) = \mathbf f_i(\mathbf x(t), \mathbf u(t), t)
$$

虚函数`computeJumpMap`定义了系统切换动态

$$
\mathbf x(t_{i+1}^+) = \mathbf j(\mathbf x(t_{i+1}))
$$

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

![cartpole.md](./not_switch_problems/cartpole.md)
