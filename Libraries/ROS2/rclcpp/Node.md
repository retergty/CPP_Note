# Node

参考文档

* [Class Node](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Node.html#classrclcpp_1_1Node)

定义在头文件`rclcpp/node.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
class Node : public std::enable_shared_from_this<Node>
```

## 常用成员函数

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

```CPP
const char *get_name() const //获取节点名，不包括名称空间
const char *get_namespace() const  //获取节点主名称空间，不包括子名称空间
RCLCPP_PUBLIC const std::string & get_sub_namespace () const //获取节点子名称空间，不包括主名称空间
RCLCPP_PUBLIC const std::string & get_effective_namespace () const //获取节点名称空间，包括主名称空间和子名称空间。
const char *get_fully_qualified_name() const // 获取节点的完全名称，包括其所在的所有名称空间。
```

### 创建发布者

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