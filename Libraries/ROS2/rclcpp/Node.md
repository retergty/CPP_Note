# Node

参考文档

* [Class Node](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Node.html#classrclcpp_1_1Node)

定义在头文件`rclcpp/node.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
class Node : public std::enable_shared_from_this<Node>
```

## 常用成员函数

对于返回共享指针的创建节点新结构的函数，**只有**返回的共享指针有效，创建的新的节点结构**才会有效**。

### 构造Node

```CPP
explicit Node(const std::string &node_name, const NodeOptions &options = NodeOptions())
```

* `node_name`是节点的名字。节点名字不能包含空格。
* `NodeOptions`是额外的选项用于控制的生成。

```CPP
explicit Node(const std::string &node_name, const std::string &namespace_, const NodeOptions &options = NodeOptions())
```

* `node_name`是节点的名字。节点名字不能包含空格。
* `namespace_`是节点所在的名称空间。不能包含空格
* `NodeOptions`是额外的选项用于控制的生成。

### 获取信息

#### 获取节点名字

```CPP
const char *get_name() const //获取节点名，不包括名称空间
const char *get_namespace() const  //获取节点主名称空间，不包括子名称空间
RCLCPP_PUBLIC const std::string & get_sub_namespace () const //获取节点子名称空间，不包括主名称空间
RCLCPP_PUBLIC const std::string & get_effective_namespace () const //获取节点名称空间，包括主名称空间和子名称空间。
const char *get_fully_qualified_name() const // 获取节点的完全名称，包括其所在的所有名称空间。
RCLCPP_PUBLIC std::vector< std::string > get_node_names () const //获取所有可用节点的完全名称，包括其所在的所有名称空间。
```

#### 获取节点信息

```CPP
rclcpp::Logger get_logger() const
```

获取节点`logger`

### 主题

#### 创建发布者

```CPP
template<typename MessageT, typename AllocatorT = std::allocator<void>, typename PublisherT = rclcpp::Publisher<MessageT, AllocatorT>>
std::shared_ptr<PublisherT> create_publisher(const std::string &topic_name, const rclcpp::QoS &qos, const PublisherOptionsWithAllocator<AllocatorT> &options = PublisherOptionsWithAllocator<AllocatorT>())
```

创建并返回`Publisher`.

* `MessageT`是发布的消息类型
* `AllocatorT`是分配内存的分配器
* `PublisherT`是发布者的类型
* `topic_name`该发布者要发布的主题名。
* `qos`是服务质量
* `options`是发布者的可选选项

```CPP
template<typename AllocatorT = std::allocator<void>>
std::shared_ptr<rclcpp::GenericPublisher> create_generic_publisher(const std::string &topic_name, const std::string &topic_type, const rclcpp::QoS &qos, const rclcpp::PublisherOptionsWithAllocator<AllocatorT> &options = (rclcpp::PublisherOptionsWithAllocator<AllocatorT>()))
```

创建并返回泛型发布者

返回的指针永远不会为空，但此函数可能会引发各种异常，例如当在`AMENT_PREFIX_PATH`上找不到消息的包时。

* `AllocatorT`是分配内存的分配器
* `topic_name`该发布者要发布的主题名。
* `topic_type`是主题的类型名
* `qos`是服务质量
* `options`是发布者的可选选项

#### 创建订阅者

```CPP
template<typename MessageT, typename CallbackT, typename AllocatorT = std::allocator<void>, typename SubscriptionT = rclcpp::Subscription<MessageT, AllocatorT>, typename MessageMemoryStrategyT = typename SubscriptionT::MessageMemoryStrategyType>
std::shared_ptr<SubscriptionT> create_subscription(const std::string &topic_name, const rclcpp::QoS &qos, CallbackT &&callback, const SubscriptionOptionsWithAllocator<AllocatorT> &options = SubscriptionOptionsWithAllocator<AllocatorT>(), typename MessageMemoryStrategyT::SharedPtr msg_mem_strat = (MessageMemoryStrategyT::create_default()))
```

创建并返回订阅

* `MessageT`是订阅的消息类型。
* `CallbackT`是回调函数类型
* `AllocatorT`是分配内存的分配器类型
* `SubscriptionT`是订阅的类型
* `MessageMemoryStrategyT`是消息内存策略类型
* `topic_name`是订阅的主题名
* `qos`是服务质量
* `callback`是订阅回调函数，收到消息后会触发这个函数
* `options`是订阅的可选选项
* `msg_mem_strat`用于分配消息内存的策略。

```CPP
template<typename CallbackT, typename AllocatorT = std::allocator<void>>
std::shared_ptr<rclcpp::GenericSubscription> create_generic_subscription(const std::string &topic_name, const std::string &topic_type, const rclcpp::QoS &qos, CallbackT &&callback, const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> &options = (rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>()))
```

创建并返回泛型订阅

* `CallbackT`是回调函数类型
* `AllocatorT`是分配内存的分配器类型
* `topic_name`是订阅的主题名
* `topic_type`是主题的类型名
* `qos`是服务质量
* `callback`是回调函数
* `options`是发布者的可选选项

#### 获取有关信息

```CPP
RCLCPP_PUBLIC size_t count_publishers (const std::string &topic_name) const //返回给定主题的发布者数量。
RCLCPP_PUBLIC size_t count_subscribers (const std::string &topic_name) const //返回给定主题的订阅者数量。
```

```CPP
RCLCPP_PUBLIC std::map< std::string, std::vector< std::string > > get_topic_names_and_types () const
```

获取所有主题，以及主题的类型的`map`.

```CPP
RCLCPP_PUBLIC 
std::vector< rclcpp::TopicEndpointInfo > get_publishers_info_by_topic (const std::string &topic_name, bool no_mangle=false) const
```

返回指定主题所有发布者及其信息。

`rclcpp::TopicEndpointInfo`包含主题端点信息，比如节点名，节点名称空间，主题类型，端点类型，主题端点`GID`,`QoS`设置。

`topic_name`可以是相对的或私有的，也可以是绝对的。相对或私有的节点名会使用当前节点名与名称空间扩展。

* `topic_name`实际使用的主题名称
* `no_mangle`为真，`topic_name`需要是合法的中间件主题名。否则它应该是有效的`ROS`主题名称。

```CPP
RCLCPP_PUBLIC 
std::vector< rclcpp::TopicEndpointInfo > get_subscriptions_info_by_topic (const std::string &topic_name, bool no_mangle=false) const
```

返回指定主题所有订阅者者及其信息。

### 创建定时器

```CPP
template<typename DurationRepT = int64_t, typename DurationT = std::milli, typename CallbackT>
rclcpp::WallTimer<CallbackT>::SharedPtr create_wall_timer(std::chrono::duration<DurationRepT, DurationT> period, CallbackT callback, rclcpp::CallbackGroup::SharedPtr group = nullptr, bool autostart = true)
```

创建一个`wall`定时器并使用`wall`时钟周期性触发回调函数

* `period`回调函数周期性触发的时间间隔
* `callback`回调函数
* `group`定时器所在的回调函数组
* `autostart`是否初始化后直接启动

```CPP
template<typename DurationRepT = int64_t, typename DurationT = std::milli, typename CallbackT>
rclcpp::GenericTimer<CallbackT>::SharedPtr create_timer(std::chrono::duration<DurationRepT, DurationT> period, CallbackT callback, rclcpp::CallbackGroup::SharedPtr group = nullptr)
```

创建一个定时器并使用节点时钟周期性地触发回调函数

* `period`回调函数周期性触发的时间间隔
* `callback`回调函数
* `group`定时器所在的回调函数组

### 服务

#### 创建客户端

```CPP
template<typename ServiceT>
rclcpp::Client<ServiceT>::SharedPtr create_client(const std::string &service_name, const rclcpp::QoS &qos = rclcpp::ServicesQoS(), rclcpp::CallbackGroup::SharedPtr group = nullptr)
```

创建客户端

* `service_name`服务的名字
* `qos`服务质量
* `group`回调函数组，处理服务调用的回复所在的回调函数组。

```CPP
rclcpp::GenericClient::SharedPtr create_generic_client(const std::string &service_name, const std::string &service_type, const rclcpp::QoS &qos = rclcpp::ServicesQoS(), rclcpp::CallbackGroup::SharedPtr group = nullptr)
```

创建泛型客户端

* `service_name`服务的名字
* `service_type`服务的类型名，比如`std_srvs/srv/SetBool`
* `qos`服务质量
* `group`回调函数组，处理服务调用的回复所在的回调函数组。

#### 创建服务端

```CPP
template<typename ServiceT, typename CallbackT>
rclcpp::Service<ServiceT>::SharedPtr create_service(const std::string &service_name, CallbackT &&callback, const rclcpp::QoS &qos = rclcpp::ServicesQoS(), rclcpp::CallbackGroup::SharedPtr group = nullptr)
```

创建服务端

* `service_name`服务的名字
* `callback`回调函数
* `qos`服务质量
* `group`回调函数组

#### 获取有关信息

```CPP
RCLCPP_PUBLIC 
std::map< std::string, std::vector< std::string > > get_service_names_and_types () const
```

获取当前所有服务与服务类型。

```CPP
RCLCPP_PUBLIC 
std::map< std::string, std::vector< std::string > > get_service_names_and_types_by_node (const std::string &node_name, const std::string &namespace_) const
```

返回特定节点所具有的（也就是在这个节点创建的服务端）服务与服务类型。

* `node_name`节点名
* `namespace_`节点所在的名称空间

```CPP
RCLCPP_PUBLIC size_t count_clients (const std::string &service_name) const //返回为给定服务创建的客户端数量。
RCLCPP_PUBLIC size_t count_services (const std::string &service_name) const //返回为给定服务创建的服务数量。
```

### 参数

#### 声明参数

```CPP
const rclcpp::ParameterValue &declare_parameter(const std::string &name, const rclcpp::ParameterValue &default_value, const rcl_interfaces::msg::ParameterDescriptor &parameter_descriptor = rcl_interfaces::msg::ParameterDescriptor(), bool ignore_override = false)
```

声明并初始化一个参数，返回有效值的引用。

该方法用于声明该节点上存在一个参数.如果在运行时，用户提供了对应参数的初始化值，则函数会使用用户提供的值覆盖，否则是默认值`default_value`。无论哪种情况，都会返回结果值，无论它是默认设置还是用户设置的。

如果没有给出`parameter_descriptor`，则将使用消息定义中的默认值，例如`read_only`将为`false`。

`rcl_interfaces::msg::ParameterDescriptor`中的名称和类型将被忽略，应使用此函数的名称参数和默认值的类型来指定。

如果`ignore_override`为`true`，则参数覆盖将被忽略。

这个方法会在设置参数时调用之前使用`add_on_set_parameters_callback`和`add_post_set_parameters_callback`设置的回调函数。

如果之前使用`add_on_set_parameters_callback`注册了回调，则将在设置节点参数之前调用该回调。如果该回调阻止的参数初始值的设置，则会抛出异常`rclcpp::exceptions::InvalidParameterValueException`。

如果之前使用`add_post_set_parameters_callback`注册了回调，则在为节点成功设置参数后将调用该回调。

此方法不会导致调用使用`add_pre_set_parameters_callback`注册的回调。

返回的引用将保持有效，直到参数被`undeclared`。

* `name`参数的名字
* `default_value`参数的默认值
* `parameter_descriptor`参数描述符
* `ignore_override`当为`true`时，参数覆盖将被忽略。默认为`false`。

* 返回值是指向参数值的常引用。

```CPP
const rclcpp::ParameterValue &declare_parameter(const std::string &name, rclcpp::ParameterType type, const rcl_interfaces::msg::ParameterDescriptor &parameter_descriptor = rcl_interfaces::msg::ParameterDescriptor{}, bool ignore_override = false)
```

声明并初始化一个参数，返回有效值。

和前一个不同的是，这个函数没有提供默认值，用户**必须**提供一个正确类型(定义在参数`type`中)的值来覆盖。

* `name`参数的名字
* `type`参数的类型
* `parameter_descriptor`参数描述符
* `ignore_override`当为`true`时，参数覆盖将被忽略。默认为`false`。

```CPP
template<typename ParameterT>
auto declare_parameter(const std::string &name, const ParameterT &default_value, const rcl_interfaces::msg::ParameterDescriptor &parameter_descriptor = rcl_interfaces::msg::ParameterDescriptor(), bool ignore_override = false)
```

使用类型声明并初始化参数。

如果默认值的类型以及返回值的类型与节点选项中提供的初始值不同，则可能会抛出`rclcpp::exceptions::InvalidParameterTypeException`

请注意，此方法无法返回`const`引用，因为扩展临时对象的生命周期只能递归地使用成员初始值设定项，并且不能扩展到返回的类的成员.此类的返回值是`ParameterValue`成员的副本.

```CPP
template<typename ParameterT>
auto declare_parameter(const std::string &name, const rcl_interfaces::msg::ParameterDescriptor &parameter_descriptor = rcl_interfaces::msg::ParameterDescriptor(), bool ignore_override = false)
```

使用类型声明并初始化参数。

这个函数没有提供默认值，用户**必须**提供一个正确类型(定义在参数`type`中)的值来覆盖。

```CPP
template<typename ParameterT>
std::vector<ParameterT> declare_parameters(const std::string &namespace_, const std::map<std::string, ParameterT> &parameters, bool ignore_overrides = false)
```

使用相同的名称空间和类型声明并初始化多个参数。

对于`map`的每项，名称为`namespace.key`的参数将被设置为`map`中的值.

名称扩展很简单，因此如果将名称空间设置为`foo.`，那么生成的参数名称将类似于`foo..key`,但是一个空的名称空间不会自动生成前导`.`.这允许您一次声明多个参数而无需命名空间。

该映射包含参数的默认值。还有另一个重载，它采用带有默认值和描述符的`std::pair`。

* `namespace_`参数所在的名称空间，可以为空。
* `parameters`要在给定命名空间中设置的参数初始值。
* `ignore_override`当为`true`时，参数覆盖将被忽略。默认为`false`。

```CPP
template<typename ParameterT>
std::vector<ParameterT> declare_parameters(const std::string &namespace_, const std::map<std::string, std::pair<ParameterT, rcl_interfaces::msg::ParameterDescriptor>> &parameters, bool ignore_overrides = false)
```

使用相同的命名空间和类型声明并初始化多个参数。

此重载使用`pair`包含初始值与描述符。

```CPP
void undeclare_parameter(const std::string &name)
```

取消声明先前声明的参数。

此方法不会调用`add_pre_set_parameters_callback`、`add_on_set_parameters_callback`和`add_post_set_parameters_callback`注册的回调。

* `name`取消声明的参数的名字。

```CPP
bool has_parameter(const std::string &name) const
```

检查有给定的参数是否已声明。

* `name`参数名

#### 设置参数

```CPP
rcl_interfaces::msg::SetParametersResult set_parameter(const rclcpp::Parameter &parameter)
```

设置指定的参数并返回结果。

如果参数尚未声明，则此函数可能会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常，但前提是节点不是在`rclcpp::NodeOptions::allow_undeclared_pa​​rameters`设置为`true`的情况下创建的。如果允许未声明的参数，则在设置参数之前会使用默认参数元数据隐式声明该参数。

此方法将导致使用`add_pre_set_parameters_callback`、`add_on_set_parameters_callback`和`add_post_set_parameters_callback` 注册的任何回调对于所设置的参数被调用一次。

如果`add_on_set_parameters_callback`回调阻止设置参数，那么它将反映在返回的`SetParametersResult`中，但不会抛出异常。

如果`add_pre_set_parameters_callback`回调使修改的参数列表为空，则会反映在返回结果中,但不会抛出异常。

`add_post_set_parameters_callback`回调会在成功设置参数后被调用。

如果参数的值类型是`rclcpp::PARAMETER_NOT_SET`，并且现有参数类型是其他类型，则该参数将隐式`undeclared`。这将导致一个参数事件，指示该参数已被删除。

* `parameter`要设置的参数。
* 返回值`SetParametersResult`表示设置的结果。

```CPP
std::vector<rcl_interfaces::msg::SetParametersResult> set_parameters(const std::vector<rclcpp::Parameter> &parameters)
```

设置给定的参数，一次一个，然后返回每个设置操作的结果。

如果由于未声明而设置参数失败，则已设置的参数将保持设置状态，并且不会尝试设置后面的参数。

这个函数将导致使用`add_pre_set_parameters_callback`、`add_on_set_parameters_callback`和`add_post_set_parameters_callback`注册的回调函数对每个参数设置都调用一次。

* `parameters`参数数组
* 返回值`std::vector<rcl_interfaces::msg::SetParametersResult>`表示设置的结果。

```CPP
rcl_interfaces::msg::SetParametersResult set_parameters_atomically(const std::vector<rclcpp::Parameter> &parameters)
```

一次性设置所有给定参数，然后汇总结果。

类似于`set_parameter`，只不过它设置多个参数，如果只有一个参数设置不成功，则所有参数都会失败。要么设置所有参数，要么不设置任何参数。

与`set_parameter 和 set_parameters`一样，如果尚未声明要设置的参数，则此方法会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常。如果抛出异常，则不会设置任何参数。

这个函数将导致使用`add_pre_set_parameters_callback`、`add_on_set_parameters_callback`和`add_post_set_parameters_callback`注册的回调函数**只会调用一次**。

如果传递多个**同名**的`rclcpp::Parameter`实例，则只会使用中的最后一个（前向迭代）。

* `parameters`参数数组
* 返回值`SetParametersResult`表示设置的结果

#### 获取参数

```CPP
rclcpp::Parameter get_parameter(const std::string &name) const
```

按给定名称返回参数。

如果参数尚未声明，则此方法可能会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常。如果参数尚未初始化，则此方法可能会抛出`rclcpp::exceptions::ParameterUninitializedException`异常。

如果允许未声明的参数,该方法不会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常，而是返回默认初始化的`rclcpp::Parameter`，其类型为`rclcpp::ParameterType::PARAMETER_NOT_SET`。

* `name`参数名
* 返回值`rclcpp::Parameter`参数

```CPP
bool get_parameter(const std::string &name, rclcpp::Parameter &parameter) const
```

通过给定名称获取参数的值，如果已设置则返回`true`。

此方法永远不会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常，但如果先前尚未声明该参数，则会返回`false`。

如果未声明参数，则该方法的输出参数（`parameter`）将不会被赋值。

* `name`参数名
* `parameter`输出的参数值
* 返回`bool`指示是否已被声明

```CPP
template<typename ParameterT>
bool get_parameter_or(const std::string &name, ParameterT &parameter, const ParameterT &alternative_value) const
```

获取参数值，如果未设置则获取另一个值`alternative_value`，并将其分配给`parameter`。

* `name`参数名
* `parameter`输出的参数值
* `alternative_value`如果未设置参数，则将值存储在输出中。
* 返回`bool`指示是否已被声明

```CPP
template<typename ParameterT>
ParameterT get_parameter_or(const std::string &name, const ParameterT &alternative_value) const
```

返回参数值，如果未设置则返回`alternative_value`。

该方法不会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常。

* `name`参数名
* `alternative_value`如果未设置参数，则将值存储在输出中。
* 返回值`rclcpp::Parameter`参数

```CPP
std::vector<rclcpp::Parameter> get_parameters(const std::vector<std::string> &names) const
```

按给定多个参数名称返回多个参数。

如果参数尚未声明，则此方法可能会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常。如果参数尚未初始化，则此方法可能会抛出`rclcpp::exceptions::ParameterUninitializedException`异常。

如果允许未声明的参数,该方法不会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常，而是返回默认初始化的`rclcpp::Parameter`，其类型为`rclcpp::ParameterType::PARAMETER_NOT_SET`。

* `names`参数名
* 返回`std::vector<rclcpp::Parameter>`参数数组

```CPP
template<typename ParameterT>
bool get_parameters(const std::string &prefix, std::map<std::string, ParameterT> &values) const
```

获取具有给定前缀`prefix`的所有参数的参数值。

生成的参数名称列表用于获取参数的值。

`map`中的`key`值会把前缀去掉，比如对于前缀`foo`和参数`foo.ping`和`foo.pong`,`map`中会包含`key`值为`ping`和`pong`。

空的前缀匹配所有的参数。

如果没有指定前缀的参数，则`values`按原样返回，且返回值为`false`.否则，对应参数会被存储在`values`中，并返回`true`.

* `prefix`前缀
* `values`输出的参数`map`.
* 返回值`bool`表示是否`values`被改变过。

#### 获取参数描述符

```CPP
rcl_interfaces::msg::ParameterDescriptor describe_parameter(const std::string &name) cons
```

返回给定参数名称的参数描述符。

与`get_parameters()`一样，如果请求的参数尚未声明且不允许使用未声明的参数，则该方法可能会抛出`rclcpp::exceptions::ParameterNotDeclaredException`异常。

如果允许未声明的参数，则将返回默认的初始化描述符。

* `name`参数名字
* `rcl_interfaces::msg::ParameterDescriptor`返回的描述符

```CPP
std::vector<rcl_interfaces::msg::ParameterDescriptor> describe_parameters(const std::vector<std::string> &names) const
```

返回多个参数描述符

#### 获取参数类型

```CPP
std::vector<uint8_t> get_parameter_types(const std::vector<std::string> &names) const
```

获取多个参数的类型

* `names`参数名字`vector`
* 返回参数的类型`std::vector<uint8_t>`

#### 获取参数表

```CPP
rcl_interfaces::msg::ListParametersResult list_parameters(const std::vector<std::string> &prefixes, uint64_t depth) const
```

返回具有任何给定前缀的参数列表，直至给定深度。

参数使用`.`进行分级结构，`prefixes`参数是一种仅选择层次结构的特定部分的方法。

* `prefixes`应在当前参数中搜索的前缀`vector`。如果此前缀向量为空，则`list_parameters`将返回所有参数。
* `depth`一个无符号整数，表示要搜索的递归深度。如果为`0`，则将返回符合前缀的所有参数。

#### 添加参数回调函数

```CPP
RCLCPP_PUBLIC RCUTILS_WARN_UNUSED 
PreSetParametersCallbackHandle::SharedPtr add_pre_set_parameters_callback (PreSetParametersCallbackType callback)
```

添加在验证参数之前触发的回调。

此回调可用于修改用户正在设置的原始参数列表，往列表中添加新的要设置的参数，删除将要设置的参数。

然后，修改后的参数列表将转发到`on set parameter`回调函数以进行验证。

每当调用任何`set_parameter*`方法或收到设置参数服务请求时，都会调用回调。

回调函数接收要设置的参数`vector`的引用。

用户应该保留返回的共享指针的副本，因为回调仅在**智能指针处于活动状态**时才有效。

传递给回调函数参数列表是原子设置产生的，也就是说`set_parameters_atomically`的参数列表才有可能多于一个要设置的参数。

当使用`declare_parameter`或`declare_parameters`声明参数时，不会调用回调。

回调函数把参数列表修改为空将导致`set_parameter*`返回`unsuccessful`.

* `callback`回调函数
* 返回值`PreSetParametersCallbackHandle::SharedPtr`.只要智能指针还有效，回调就有效。

```CPP
RCLCPP_PUBLIC RCUTILS_WARN_UNUSED 
OnSetParametersCallbackHandle::SharedPtr add_on_set_parameters_callback (OnSetParametersCallbackType callback)
```

添加在设置参数之前触发的回调。

该回调旨在当上述任何`set_parameter*`或`declare_parameter*`函数设置参数前调用。允许节点开发人员控制当前情况下可以哪些参数可以更改。

该回调函数接收要设置参数`vector`的`const`引用，并返回`rcl_interfaces::msg::SetParametersResult`指示是否应设置该参数,并可声明失败原因。

用户应该保留返回的共享指针的副本，因为回调仅在**智能指针处于活动状态**时才有效。

注意该回调函数也会在`declare_parameter`是触发，所以开发者不能假定该回调函数调用时，指定参数已经设置，因此，在根据现有值检查新值时，必须考虑参数尚未设置的情况。

当使用`undeclare_parameter`取消声明参数时，不会调用该回调。

设置参数时会调用注册的回调。当回调返回不成功的结果时，不会调用其余的回调。

* `callback`回调函数
* 返回值`OnSetParametersCallbackHandle::SharedPtr`.只要智能指针还有效，回调就有效。

```CPP
RCLCPP_PUBLIC RCUTILS_WARN_UNUSED 
PostSetParametersCallbackHandle::SharedPtr add_post_set_parameters_callback (PostSetParametersCallbackType callback)
```

添加参数设置成功后触发的回调。

当任何`set_parameter*`或`declare_parameter*`方法成功时，将调用该回调。

该回调函数接收成功设置参数`vector`的`const`引用。

该回调可能很有价值，因为它可以根据参数更改产生副作用，例如，一旦参数成功更改，就更新内部跟踪的类属性。

当使用`undeclare_parameter`取消声明参数时，不会调用该回调。

* `callback`回调函数
* 返回值`PostSetParametersCallbackHandle::SharedPtr`.只要智能指针还有效，回调就有效。

#### 删除参数回调函数

```CPP
RCLCPP_PUBLIC void remove_pre_set_parameters_callback (const PreSetParametersCallbackHandle *const handler)
RCLCPP_PUBLIC void remove_on_set_parameters_callback (const OnSetParametersCallbackHandle *const handler)
RCLCPP_PUBLIC void remove_post_set_parameters_callback (const PostSetParametersCallbackHandle *const handler)
```

使用函数把指定`handler`从回调中删除。

也可以使用添加回调函数时的智能指针。把所有指向它的智能指针删除。

```CPP
callback_shared_ptr.reset()
```

### 子节点

```CPP
RCLCPP_PUBLIC 
rclcpp::Node::SharedPtr create_sub_node (const std::string &sub_namespace)
```

创建子节点(sub-node)，在子节点上创建的结构会扩展名称空间

子节点（从属节点的缩写）是此类的一个实例，它使用此类的现有实例创建，具有附加的子命名空间.

默认情况下，当节点被创建时，它没有与之关联的子节点于子名称空间。可以使用该函数创建子节点。

子节点也可以创建子节点，这种情况下，新的子节点名称空间在原有的子名称空间上继续拓展。

* `sub_namespace`子节点的子名称空间，必须是相对的，以`/`的绝对名称空间会抛出异常。
* 返回值`rclcpp::Node::SharedPtr`指向子节点的共享指针。
