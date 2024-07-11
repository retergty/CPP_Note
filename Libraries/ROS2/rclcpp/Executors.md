# 执行器Executors

执行器进行阻塞自旋(blocking spin)，阻塞线程并判断执行回调函数

## Executor类

定义在头文件`rclcpp/executors.hpp`的`rclcpp`名称空间中。

参考文档

* [Class Executor](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Executor.html#exhale-class-classrclcpp-1-1executor)

```CPP
class Executor
```

### 派生类型

* `public rclcpp::executors::MultiThreadedExecutor`
* `public rclcpp::executors::SingleThreadedExecutor`
* `public rclcpp::executors::StaticSingleThreadedExecutor`
* `public rclcpp::experimental::executors::EventsExecutor`

### 描述

协调可用通信任务的顺序和时间安排。

`Executor`提供了自旋函数(`spin_node_once`,`spin_some`).它查找节点与回调函数组中可用的工作并执行，过程基于派生类实现的线程与并发策略。常见的可用工作是执行订阅回调(subscription callback)或计时器回调(timer callback)。`Executor`的架构使得解耦通信流图与处理模型成为可能。

### 常用成员函数

#### 构造与析构函数

```CPP
explicit Executor(const rclcpp::ExecutorOptions &options = rclcpp::ExecutorOptions())
virtual ~Executor()
```

#### 连接节点

```CPP
virtual void add_node(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr, bool notify = true)
virtual void add_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify = true)
```

将节点加入到`Executor`中。

节点有与之关联的回调函数组，这个函数会把使用`automatically_add_to_executor_with_node`参数为真的关联`CallbackGroup`加入到`Executor`.该节点也会与执行器关联，以便将来在节点上创建且将`automatic_add_to_executor_with_node`参数设置为`true`的回调组也会自动与该执行器关联。

`automatically_add_to_executor_with_node`参数为假的关联`CallbackGroup`必须使用`add_callback_group`把回调函数组加入到`Executor`。

如果节点已经与执行器关联，则此方法将抛出异常。

* `node_ptr`节点指针
* `notify`通知执行器并唤醒，是否通知执行器有新的结构加入，如果为`false`则需要等待其自动发现。

```CPP
virtual void remove_node(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr, bool notify = true)
virtual void remove_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify = true)
```

移去节点，任何与该节点相关的自动添加的回调函数组也会被移去，并且该节点不再与该`Executor`关联。

* `node_ptr`节点指针
* `notify`通知执行器并唤醒，如果在执行器被阻塞等待另一个线程中的工作时从执行器中删除最后一个节点，则这非常有用，因为否则执行器将永远不会收到通知。

#### 操作回调函数组

```CPP
virtual void add_callback_group(rclcpp::CallbackGroup::SharedPtr group_ptr, rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr, bool notify = true)
```

把回调函数组加入到`Executor`，如果`CallbackGroup`在创建时指定了自动添加到执行器，则不需要手动调用这个函数。

一个`Executor`可以有零个或者多个回调函数组，在`spin`函数中工作。`Executor`会检查要加入的回调函数组是否已经加入到了另外的`Executor`中，如果已经加入了，就产生异常。

使用这个函数把回调函数组不会以任何方式将它的节点与此`Executor`关联。这个函数使得一个节点的不同回调函数可以被**不同**的`Executor`执行。

* `group_ptr`回调函数组指针。
* `node_ptr`节点指针。由于`ROS2`架构，是通过节点指针的订阅结构来发现回调函数组的。
* `notify`通知执行器并唤醒，是否通知执行器有新的结构加入，如果为`false`则需要等待其自动发现。

```CPP
virtual std::vector<rclcpp::CallbackGroup::WeakPtr> get_all_callback_groups()
```

获取属于`Executor`的回调函数组。

该函数返回一个`weak`指针向量，这些指针指向与执行器关联的回调组。与执行器关联的回调函数组就是之前使用`add_node`，`add_callback_group`或者是自动由关联节点关联的回调函数组。

* `std::vector<rclcpp::CallbackGroup::WeakPtr>`返回指向回调函数组的`vector`.

```CPP
virtual std::vector<rclcpp::CallbackGroup::WeakPtr> get_manually_added_callback_groups()
```

获取手动添加进入`Executor`的回调函数组,也就是调用`add_callback_group`加入的回调函数组。

```CPP
virtual std::vector<rclcpp::CallbackGroup::WeakPtr> get_automatically_added_callback_groups_from_nodes()
```

获取自动添加进入`Executor`的回调函数组。

```CPP
virtual void remove_callback_group(rclcpp::CallbackGroup::SharedPtr group_ptr, bool notify = true)
```

把特定回调函数组从`Executor`中移去。

回调组从执行器中移除并解除关联。如果这个回调函数组还是某个节点最后与`Executor`关联的回调函数组，则还会触发中断条件把这个节点给失能。

此函数仅删除使用`rclcpp::Executor::add_callback_group`手动添加的回调组。若要把自动添加的回调函数组删除，请使用`remove_node`.

* `group_ptr`指向回调函数组的指针
* `notify`通知执行器并唤醒，是否通知执行器有回调函数被删除。

#### 自旋

```CPP
virtual void spin() = 0
```

纯虚函数，阻止该类实例化。

```CPP
template<typename RepT = int64_t, typename T = std::milli>
inline void spin_node_once(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node, std::chrono::duration<RepT, T> timeout = std::chrono::duration<RepT, T>(-1))
template<typename NodeT = rclcpp::Node, typename RepT = int64_t, typename T = std::milli>
inline void spin_node_once(std::shared_ptr<NodeT> node, std::chrono::duration<RepT, T> timeout = std::chrono::duration<RepT, T>(-1))
```

将节点添加到执行器，在超时时间内执行**下一个**可用的工作单元，如果有多个可用的工作，也只会执行一个，然后删除该节点。

* `node`指向节点的指针
* `timeout`等待可用的工作的时间，负数表示无限等待，0表示不等待，只执行目前可用的任务。

```CPP
virtual void spin_node_some(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node)
virtual void spin_node_some(std::shared_ptr<rclcpp::Node> node)
```

添加节点，完成所有立即可用的工作，然后删除节点.

* `node`指向节点的指针

```CPP
virtual void spin_some(std::chrono::nanoseconds max_duration = std::chrono::nanoseconds(0))
```

收集工作一次并执行所有可用工作，在可选的最长的持续时间内执行。

如果添加了具有阻塞或者是长运行时的回调函数，这个函数的运行时间可能会显著超出`max_duration`.

如果调用时没有工作要做，它将立即返回，因为可用工作的收集是非阻塞的。执行每个准备工作之前，该函数检查是否超过了`max_duration`，如果超过，则立即返回，不执行下一个工作。

* `max_duration`最长持续时间，为`0`则意味着无限持续时间。

```CPP
virtual void spin_node_all(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node, std::chrono::nanoseconds max_duration)
virtual void spin_node_all(std::shared_ptr<rclcpp::Node> node, std::chrono::nanoseconds max_duration)
```

将节点添加到执行器，在超时时间内执行**所有**可用的工作单元，然后删除该节点。

* `node`指向节点的指针
* `max_duration`最长持续时间

```CPP
virtual void spin_all(std::chrono::nanoseconds max_duration)
```

在一段时间`max_duration`内**重复收集和执行工作**，或者直到没有更多工作可用。

添加具有阻塞回调的订阅、计时器、服务等将导致该函数阻塞（这可能会产生意想不到的后果）。

如果执行等待对象所需的时间长于新等待对象准备就绪的时间，则此方法将重复执行工作，直到`max_duration`过去。

* `max_duration`最长持续时间，`0`表示着无限持续时间。

```CPP
virtual void spin_once(std::chrono::nanoseconds timeout = std::chrono::nanoseconds(-1))
```

在超时时间内执行**下一个**可用的工作单元，如果有多个可用的工作，也只会执行一个

* `timeout`等待可用的工作的时间，负数表示无限等待，0表示不等待，只执行目前可用的任务。

```CPP
template<typename FutureT, typename TimeRepT = int64_t, typename TimeT = std::milli>
inline FutureReturnCode spin_until_future_complete(const FutureT &future, std::chrono::duration<TimeRepT, TimeT> timeout = std::chrono::duration<TimeRepT, TimeT>(-1))
```

自旋（阻塞）直到`future`完成、等待超时、或者rclcpp被中断。

* `future`等待的未来,如果此函数返回`SUCCESS`，则可以在不阻塞的情况下访问`future`（尽管它仍可能抛出异常）。
* `timeout`等待最长时间，`-1`表示无限等待，`0`表示不阻塞。
* `FutureReturnCode`返回值，`SUCCESS`, `INTERRUPTED`, 或`TIMEOUT`三者之一

```CPP
virtual void cancel()
```

取消所有的自旋函数，引起这些自选函数返回。

可以在**任何线程异步调用**。

```CPP
bool is_spinning()
```

如果执行器当前正在自旋，则返回`true`。

可以在**任何线程异步调用**。

## SingleThreadedExecutor类

定义在头文件`rclcpp/executors.hpp`的`rclcpp`名称空间中。

参考文档

* [Class SingleThreadedExecutor](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1executors_1_1SingleThreadedExecutor.html#exhale-class-classrclcpp-1-1executors-1-1singlethreadedexecutor)

```CPP
class SingleThreadedExecutor : public rclcpp::Executor
```

### 描述

单线程执行器，是`rclcpp::spin`函数默认生成的执行器。

### 常用成员函数

#### 构造与析构

```CPP
explicit SingleThreadedExecutor(const rclcpp::ExecutorOptions &options = rclcpp::ExecutorOptions())
virtual ~SingleThreadedExecutor()
```

构造函数与析构函数

#### 自旋

```CPP
virtual void spin() override
```

单线程执行器自旋的实现。

这个函数会阻塞当前线程，等待工作到来并处理工作，如此往复，直到被取消。它可以被`rclcpp::Executor::cancel()`成员函数取消，也可以在`SIGINT`消息被取消（命令行操作是`ctrl-c`）。

## StaticSingleThreadedExecutor类

定义在头文件`rclcpp/executors.hpp`的`rclcpp`名称空间中。

参考文档

* [Class StaticSingleThreadedExecutor](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1executors_1_1StaticSingleThreadedExecutor.html#exhale-class-classrclcpp-1-1executors-1-1staticsinglethreadedexecutor)

```CPP
class StaticSingleThreadedExecutor : public rclcpp::Executor
```

### 描述

单线程静态执行器。它是静态的，因为它不会为每次迭代重建可执行列表。所有节点、回调组、计时器、订阅等都是在调用`spin()`之前创建的，并且仅在向节点添加/删除实体时才进行修改。

### 常用成员函数

#### 构造与析构

```CPP
explicit StaticSingleThreadedExecutor(const rclcpp::ExecutorOptions &options = rclcpp::ExecutorOptions())
virtual ~StaticSingleThreadedExecutor()
```

#### 自旋

```CPP
virtual void spin() override
```

静态单线程执行器自旋的实现。

这个函数会阻塞当前线程，等待工作到来并处理工作，如此往复，直到被取消。它可以被`rclcpp::Executor::cancel()`成员函数取消，也可以在`SIGINT`消息被取消（命令行操作是`ctrl-c`）。

```CPP
virtual void spin_some(std::chrono::nanoseconds max_duration = std::chrono::nanoseconds(0)) override
```

此非阻塞函数将执行调用此`API`时已准备好的工作，直到超时或没有更多可用工作.如果在执行工作时，有新的工作可用，也不会纳入考虑。

注意此函数的非阻塞特性，必要时需要加入`sleep`来防止CPU过度占用。

```CPP
virtual void spin_all(std::chrono::nanoseconds max_duration) override
```

此非阻塞函数将执行调用此`API`时已准备好的工作，直到超时或没有更多可用工作.如果在执行工作时，有新的工作可用，也会纳入考虑。

## MultiThreadedExecutor类

定义在头文件`rclcpp/executors.hpp`的`rclcpp`名称空间中。

参考文档

* [Class MultiThreadedExecutor](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1executors_1_1MultiThreadedExecutor.html#exhale-class-classrclcpp-1-1executors-1-1multithreadedexecutor)

```CPP
class MultiThreadedExecutor : public rclcpp::Executor
```

### 描述

多线程执行器，采取线程池技术并发地执行回调函数组。

### 常见成员函数

#### 构造和析构

```CPP
explicit MultiThreadedExecutor(const rclcpp::ExecutorOptions &options = rclcpp::ExecutorOptions(), size_t number_of_threads = 0, bool yield_before_execute = false, std::chrono::nanoseconds timeout = std::chrono::nanoseconds(-1))
```

构造多线程执行器

* `options`是执行器选项。
* `number_of_threads`是线程池中的线程数，`0`意味着使用能使用的CPU核心数
* `yield_before_execute`为真则会在获取完毕工作后调用`std::this_thread::yield()`,在执行工作前。这对于重现与获取同一个工作多次有关的错误很有用。

#### 自旋

```CPP
virtual void spin() override
```

自旋

```CPP
size_t get_number_of_threads()
```

获取线程个数

## CallbackGroup类

定义在`rclcpp/callback_group.hpp`中

参考文档

* [Class CallbackGroup](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1CallbackGroup.html#classrclcpp_1_1CallbackGroup)

### 描述

回调函数组类，用于创建回调函数组。

### 常见成员函数

#### 构造与析构

```CPP
explicit CallbackGroup(CallbackGroupType group_type, bool automatically_add_to_executor_with_node = true)
```

回调函数组有类型，互斥`Mutually Exclusive`与可重入`Reentrant`。

可重入的回调函数组中的回调函数必须满足

* 与自身同时运行(可重入)
* 与组中的其他回调同时运行
* 与其他组中的其他回调同时运行

互斥的回调函数组中的回调函数必须满足

* 不会同时运行多次（不可重入）
* 不会与组中的其他回调同时运行
* 但必须可以与其他组中的回调同时运行

* `group_type`回调函数组类型。
* `automatically_add_to_executor_with_node`这个决定回调函数组是否会随着它所关联的节点一起加入执行器中。节点是在创建回调组之前还是之后添加到执行器中是无关紧要的；无论哪种情况，回调组都会自动添加到执行器中。

```CPP
explicit CallbackGroup(CallbackGroupType group_type, rclcpp::Context::WeakPtr context, bool automatically_add_to_executor_with_node = true)
```

* `context`上下文。

#### 获取信息

```CPP
template<typename Function>
inline rclcpp::SubscriptionBase::SharedPtr find_subscription_ptrs_if(Function func) const
template<typename Function>
inline rclcpp::TimerBase::SharedPtr find_timer_ptrs_if(Function func) const
template<typename Function>
inline rclcpp::ServiceBase::SharedPtr find_service_ptrs_if(Function func) const
template<typename Function>
inline rclcpp::ClientBase::SharedPtr find_client_ptrs_if(Function func) const
template<typename Function>
inline rclcpp::Waitable::SharedPtr find_waitable_ptrs_if(Function func) const
```

查找特定类型的函数是否在特定的实体中，若在，则返回。

* `Function`要查找的函数。
* `returnVal`返回指向查找到的函数指针。

```CPP
size_t size() const
```

获取该回调函数组的所含的回调函数总数。

* `size_t`回调组中的实体数量。

```CPP
std::atomic_bool &can_be_taken_from()
```

返回一个原子布尔值的引用，表示可以被拿走。

返回值为真意味着没有执行器当前正在使用这个回调函数组中的实体。返回值为假意味着如果执行程序当前正在使用该组中的可执行实体，并且组策略不允许第二次执行（例如互斥）。

```CPP
const CallbackGroupType &type() const
```

得到回调函数组类型

```CPP
std::atomic_bool &get_associated_with_executor_atomic()
```

返回一个原子布尔值引用，表示与执行器相连。

当回调组添加到执行器时，将检查此布尔值以确保它尚未添加到另一个执行器。当回调组加入到执行器后，则将此布尔值设置为`true`以指示它现在与执行程序关联。

当回调组从执行器中删除时，该原子布尔值将设置回`false`。

```CPP
bool automatically_add_to_executor_with_node() const
```

如果此回调组应由节点自动添加到执行器，则返回`true`。

#### 获取函数

```CPP
void collect_all_ptrs(std::function<void(const rclcpp::SubscriptionBase::SharedPtr&)> sub_func, std::function<void(const rclcpp::ServiceBase::SharedPtr&)> service_func, std::function<void(const rclcpp::ClientBase::SharedPtr&)> client_func, std::function<void(const rclcpp::TimerBase::SharedPtr&)> timer_func, std::function<void(const rclcpp::Waitable::SharedPtr&)> waitable_func) const
```

收集此回调组中包含的所有实体指针。

* `sub_func`为每个订阅执行的函数
* `service_func`为每个服务执行的函数
* `client_func`为每个客户端执行的函数
* `timer_func`为每个定时器执行的函数
* `waitable_fuinc`为每个等待执行的函数

## 自旋API

定义在头文件`rclcpp/executors.hpp`的`rclcpp`名称空间中。

参考文档

* [spin](https://docs.ros.org/en/jazzy/p/rclcpp/generated/function_namespacerclcpp_1a21e13577f5bcc5992de1d7dd08d8652b.html#namespacerclcpp_1a21e13577f5bcc5992de1d7dd08d8652b)
* [spin_some](https://docs.ros.org/en/jazzy/p/rclcpp/generated/function_namespacerclcpp_1ad48c7a9cc4fa34989a0849d708d8f7de.html#namespacerclcpp_1ad48c7a9cc4fa34989a0849d708d8f7de)
* [spin_until_future_complete](https://docs.ros.org/en/jazzy/p/rclcpp/generated/function_namespacerclcpp_1ab83b41b70748bbd4631b498596148360.html#namespacerclcpp_1ab83b41b70748bbd4631b498596148360)

```CPP
void rclcpp::spin(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr)
```

* [What does rclcpp::spin actually do ? What is "spin node" mean?](https://answers.ros.org/question/388589/what-does-rclcppspin-actually-do-what-is-spin-node-mean/)

创建一个默认的单线程执行器并自旋指定节点。等价于调用

```CPP
rclcpp::executors::SingleThreadedExecutor executor;
executor.add_node(node);
executor.spin();
```

* `node_ptr`是节点指针。

```CPP
void rclcpp::spin_some(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr)
```

创建一个默认的单线程执行器并执行任何立即可用的工作。

* `node_ptr`是节点指针。

```CPP
template<typename FutureT, typename TimeRepT = int64_t, typename TimeT = std::milli>
rclcpp::FutureReturnCode rclcpp::spin_until_future_complete(rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_ptr, const FutureT &future, std::chrono::duration<TimeRepT, TimeT> timeout = std::chrono::duration<TimeRepT, TimeT>(-1))
```

自旋直到`future`可用。
