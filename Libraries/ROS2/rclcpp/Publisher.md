# Publisher

参考文档

* [Template Class Publisher](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Publisher.html#classrclcpp_1_1Publisher)

定义在头文件`rclcpp/publisher.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
template<typename MessageT, typename AllocatorT = std::allocator<void>>
class Publisher : public rclcpp::PublisherBase
```

* `MessageT`必须是
  * `ROS`消息类型,例如 std_msgs::msgs::String，
  * `rclcpp::TypeAdapter<CustomType, ROSMessageType>`
  * 已使用`RCLCPP_USING_CUSTOM_TYPE_AS_ROS_MESSAGE_TYPE(custom_type, ros_message_type)`设置为`ROS`类型的隐式类型的自定义类型
* `AllocatorT`是分配器

## 常用成员函数

### 构造与析构

```CPP
inline Publisher(rclcpp::node_interfaces::NodeBaseInterface *node_base, const std::string &topic, const rclcpp::QoS &qos, const rclcpp::PublisherOptionsWithAllocator<AllocatorT> &options)
```

构造函数，通常不是直接调用的，而是使用`rclcpp::create_publisher()`构建并获取的。

* `node_base`发布消息的节点
* `topic`要发布到的主题
* `qos`通信质量
* `options`可选项

### 发布

```CPP
template<typename T>
inline std::enable_if_t<rosidl_generator_traits::is_message<T>::value && std::is_same<T, ROSMessageType>::value> publish(std::unique_ptr<T, ROSMessageTypeDeleter> msg)
template<typename T>
inline std::enable_if_t<rclcpp::TypeAdapter<MessageT>::is_specialized::value && std::is_same<T, PublishedType>::value> publish(std::unique_ptr<T, PublishedTypeDeleter> msg)
```

发布一个消息。

此重载函数允许用户将消息的所有权授予`rclcpp`，从而实现更高效的进程内通信优化.

```CPP
template<typename T>
inline std::enable_if_t<rosidl_generator_traits::is_message<T>::value && std::is_same<T, ROSMessageType>::value> publish(const T &msg)

template<typename T>
inline std::enable_if_t<rclcpp::TypeAdapter<MessageT>::is_specialized::value && std::is_same<T, PublishedType>::value> publish(const T &msg)
```

发布一个消息

此重载函数允许用户提供对消息的引用，该消息将不加修改地复制到堆上，以便`rclcpp`可以拥有副本，并且可以在以后需要时移动这个副本的所有权。

```CPP
inline void publish(const rcl_serialized_message_t &serialized_msg)
inline void publish(const SerializedMessage &serialized_msg)
```
