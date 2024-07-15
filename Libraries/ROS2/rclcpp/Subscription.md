# Subscription

参考文档

* [Template Class Subscription](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Subscription.html#classrclcpp_1_1Subscription)

定义在头文件`rclcpp/subscription.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
template<typename MessageT, typename AllocatorT = std::allocator<void>, typename SubscribedT = typename rclcpp::TypeAdapter<MessageT>::custom_type, typename ROSMessageT = typename rclcpp::TypeAdapter<MessageT>::ros_message_type, typename MessageMemoryStrategyT = rclcpp::message_memory_strategy::MessageMemoryStrategy<ROSMessageT, AllocatorT>>
class Subscription : public rclcpp::SubscriptionBase
```

## 常用成员函数

### 构造与析构

```CPP
inline Subscription(rclcpp::node_interfaces::NodeBaseInterface *node_base, const rosidl_message_type_support_t &type_support_handle, const std::string &topic_name, const rclcpp::QoS &qos, AnySubscriptionCallback<MessageT, AllocatorT> callback, const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> &options, typename MessageMemoryStrategyT::SharedPtr message_memory_strategy, SubscriptionTopicStatisticsSharedPtr subscription_topic_statistics = nullptr)
```

构造函数，通常不是直接调用的，而是使用`rclcpp::create_subscription()`构建并获取的。

* `node_base`订阅消息的节点
* `type_support_handle`，`rosidl type support`结构，用于主题的消息。
* `topic_name`订阅的主题名
* `qos`通信质量
* `callback`当消息接收后触发的回调函数
* `options`订阅的选项
* `message_memory_strategy`用于管理消息内存的内存策略
* `subscription_topic_statistics`指向主题统计订阅的可选指针

### 获取消息

```CPP
inline bool take(ROSMessageType &message_out, rclcpp::MessageInfo &message_info_out)
```

从进程间订阅中获取下一条消息。

即使返回`false`，数据也可能写入到了`message_out`和`message_info_out`中。特别是在删除冗余进程内数据的情况下,这种情况下数据通过跨进程与进程间接收，由于底层中间件无法避免这种重复传递，因此，来自这些进程内发布者的进程间数据将被忽略，但必须首先`take`才能知道它是进程内通信者，才可以被丢弃。

* `message_out`把消息复制到其中
* `message_info_out`获取的消息的消息信息
* 返回值`bool`指示数据的有效性

```CPP
template<typename TakeT>
inline std::enable_if_t<!rosidl_generator_traits::is_message<TakeT>::value && std::is_same_v<TakeT, SubscribedType>, bool> take(TakeT &message_out, rclcpp::MessageInfo &message_info_out)
```

从进程间订阅中获取下一条消息

### 检查消息

```CPP
inline virtual void handle_message(std::shared_ptr<void> &message, const rclcpp::MessageInfo &message_info) override
```

检查是否需要处理这个消息，如果是，调用回调函数。

## 回调函数

回调函数的类型如下

```CPP
  using ConstRefCallback =
    std::function<void (const SubscribedType &)>;
  using ConstRefROSMessageCallback =
    std::function<void (const ROSMessageType &)>;
  using ConstRefWithInfoCallback =
    std::function<void (const SubscribedType &, const rclcpp::MessageInfo &)>;
  using ConstRefWithInfoROSMessageCallback =
    std::function<void (const ROSMessageType &, const rclcpp::MessageInfo &)>;
  using ConstRefSerializedMessageCallback =
    std::function<void (const rclcpp::SerializedMessage &)>;
  using ConstRefSerializedMessageWithInfoCallback =
    std::function<void (const rclcpp::SerializedMessage &, const rclcpp::MessageInfo &)>;

  using UniquePtrCallback =
    std::function<void (std::unique_ptr<SubscribedType, SubscribedMessageDeleter>)>;
  using UniquePtrROSMessageCallback =
    std::function<void (std::unique_ptr<ROSMessageType, ROSMessageDeleter>)>;
  using UniquePtrWithInfoCallback =
    std::function<void (
        std::unique_ptr<SubscribedType, SubscribedMessageDeleter>,
        const rclcpp::MessageInfo &)>;
  using UniquePtrWithInfoROSMessageCallback =
    std::function<void (
        std::unique_ptr<ROSMessageType, ROSMessageDeleter>,
        const rclcpp::MessageInfo &)>;
  using UniquePtrSerializedMessageCallback =
    std::function<void (std::unique_ptr<rclcpp::SerializedMessage, SerializedMessageDeleter>)>;
  using UniquePtrSerializedMessageWithInfoCallback =
    std::function<void (
        std::unique_ptr<rclcpp::SerializedMessage, SerializedMessageDeleter>,
        const rclcpp::MessageInfo &)>;

  using SharedConstPtrCallback =
    std::function<void (std::shared_ptr<const SubscribedType>)>;
  using SharedConstPtrROSMessageCallback =
    std::function<void (std::shared_ptr<const ROSMessageType>)>;
  using SharedConstPtrWithInfoCallback =
    std::function<void (
        std::shared_ptr<const SubscribedType>,
        const rclcpp::MessageInfo &)>;
  using SharedConstPtrWithInfoROSMessageCallback =
    std::function<void (
        std::shared_ptr<const ROSMessageType>,
        const rclcpp::MessageInfo &)>;
  using SharedConstPtrSerializedMessageCallback =
    std::function<void (std::shared_ptr<const rclcpp::SerializedMessage>)>;
  using SharedConstPtrSerializedMessageWithInfoCallback =
    std::function<void (
        std::shared_ptr<const rclcpp::SerializedMessage>,
        const rclcpp::MessageInfo &)>;

  using ConstRefSharedConstPtrCallback =
    std::function<void (const std::shared_ptr<const SubscribedType> &)>;
  using ConstRefSharedConstPtrROSMessageCallback =
    std::function<void (const std::shared_ptr<const ROSMessageType> &)>;
  using ConstRefSharedConstPtrWithInfoCallback =
    std::function<void (
        const std::shared_ptr<const SubscribedType> &,
        const rclcpp::MessageInfo &)>;
  using ConstRefSharedConstPtrWithInfoROSMessageCallback =
    std::function<void (
        const std::shared_ptr<const ROSMessageType> &,
        const rclcpp::MessageInfo &)>;
  using ConstRefSharedConstPtrSerializedMessageCallback =
    std::function<void (const std::shared_ptr<const rclcpp::SerializedMessage> &)>;
  using ConstRefSharedConstPtrSerializedMessageWithInfoCallback =
    std::function<void (
        const std::shared_ptr<const rclcpp::SerializedMessage> &,
        const rclcpp::MessageInfo &)>;
```

分为几个部分，大致为接受`const`引用的回调函数，接受`unique_ptr`的回调函数，接受`shared_ptr`的回调函数。

所有的回调函数都是接受目前从主题订阅到的消息的类型，或者是消息信息的类型。

使用`const`引用的回调函数把不会修改消息，回调函数调用结束后不会显式释放消息内存。

使用`unique_ptr`的回调函数可以修改消息，回调函数调用结束后释放消息内存。

使用`shared_ptr`的回调函数可以修改消息，可以延长，多线程使用消息。

推荐使用`unique_ptr`.
