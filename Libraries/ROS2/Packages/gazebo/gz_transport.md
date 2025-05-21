# gazebo transport

`gz_transport`是`gazebo`的原生数据交换设施，可以发布接收`gazebo`的消息，控制`gazebo`仿真，获取传感器数据等.

`gazebo transport`使用`protobuf`作为消息的生成工具。

参考文档

* [Gazebo Transport API Reference](https://gazebosim.org/api/transport/14/introduction.html)
* [github gz_transport](https://github.com/gazebosim/gz-transport)

## Node and Topic

参考文档

* [Nodes and Topics](https://gazebosim.org/api/transport/14/nodestopics.html)

### Node

`Gazebo Transport`中的通信遵循纯分布式架构.网络中的所有节点都可以充当发布者、订阅者，提供服务和请求服务.

类似于`ROS2`通信架构，`gazebo`也提供了发布订阅`publish/subscribe`，服务`service`的结构.

### Topic

类似于`ROS2`的主题.

下面是一些`Topics`例子.

* `/topicA`
* `/topicA/`,等价于上者
* `topicA`
* `/a/b`

同样的`Topics`也可以有名称空间.以`/`开头的不会附上当前名称空间.名称空间由节点设置.

## Service

类似于`ROS2`的`Services`.

## 常见API

### Node

#### Advertise

用于声明主题，服务,取决于传递的实参.

声明主题.

```CPP
template<typename MessageT >
Node::Publisher  Advertise (const std::string &_topic, const AdvertiseMessageOptions &_options=AdvertiseMessageOptions())
  //Advertise a new topic. If a topic is currently advertised, you cannot advertise it a second time (regardless of its type).
 
Node::Publisher  Advertise (const std::string &_topic, const std::string &_msgTypeName, const AdvertiseMessageOptions &_options=AdvertiseMessageOptions())
  //Advertise a new topic. If a topic is currently advertised, you cannot advertise it a second time (regardless of its type).
```

声明服务，包含没有输入或输出的服务.

```CPP
template<typename RequestT , typename ReplyT >
bool  Advertise (const std::string &_topic, std::function< bool(const RequestT &_request, ReplyT &_reply)> _callback, const AdvertiseServiceOptions &_options=AdvertiseServiceOptions())
  //Advertise a new service. In this version the callback is a lambda function.
 
template<typename ReplyT >
bool  Advertise (const std::string &_topic, std::function< bool(ReplyT &_reply)> &_callback, const AdvertiseServiceOptions &_options=AdvertiseServiceOptions())
  //Advertise a new service without input parameter. In this version the callback is a lambda function.
 
template<typename RequestT >
bool  Advertise (const std::string &_topic, std::function< void(const RequestT &_request)> &_callback, const AdvertiseServiceOptions &_options=AdvertiseServiceOptions())
 // Advertise a new service without any output parameter. In this version the callback is a lambda function.
```

随后可以使用.

```CPP
bool Publish (const ProtoMsg &_msg)
```

发布到主题

#### Subscribe

用于订阅主题.订阅回调函数.

```CPP
template<typename MessageT >
bool  Subscribe (const std::string &_topic, std::function< void(const MessageT &_msg)> _callback, const SubscribeOptions &_opts=SubscribeOptions())
 //Subscribe to a topic registering a callback. Note that this callback does not include any message information. In this version the callback is a lambda function.
 
template<typename MessageT >
bool  Subscribe (const std::string &_topic, std::function< void(const MessageT &_msg, const MessageInfo &_info)> _callback, const SubscribeOptions &_opts=SubscribeOptions())
  //Subscribe to a topic registering a callback. Note that this callback includes message information. In this version the callback is a lambda function.
```

#### Request

用于请求服务.

```CPP
template<typename RequestT >
bool  Request (const std::string &_topic, const RequestT &_request)
  //Request a new service without waiting for response.
 
template<typename RequestT , typename ReplyT >
bool  Request (const std::string &_topic, const RequestT &_request, const unsigned int &_timeout, ReplyT &_reply, bool &_result)
  //Request a new service using a blocking call.
 
template<typename RequestT , typename ReplyT >
bool  Request (const std::string &_topic, const RequestT &_request, std::function< void(const ReplyT &_reply, const bool _result)> &_callback)
  //Request a new service using a non-blocking call. In this version the callback is a lambda function.
```

## 常见消息

### EntityFactory

```protobuf
message EntityFactory
{
  /// \brief Optional header data
  Header header                    = 1;

  /// \brief Only one method is supported at a time
  oneof from
  {
    /// \brief SDF description in string format.
    string sdf                     = 2;

    /// \brief Full path to SDF file.
    string sdf_filename            = 3;

    /// \brief Description of model to be inserted.
    Model model                    = 4;

    /// \brief Description of light to be inserted.
    Light light                    = 5;

    /// \brief Name of entity to clone.
    string clone_name              = 6;
  }

  /// \brief Pose where the entity will be spawned in the world.
  /// If set, `spherical_coordinates` will be ignored.
  Pose pose                        = 7;

  /// \brief New name for the entity, overrides the name on the SDF.
  string name                      = 8;

  /// \brief Whether the server is allowed to rename the entity in case of
  /// overlap with existing entities.
  bool allow_renaming              = 9;

  /// \brief The pose will be defined relative to this frame. If left empty,
  /// the "world" frame will be used.
  string relative_to               = 10;

  /// \brief Spherical coordinates where the entity will be spawned in the
  /// world.
  /// If `pose` is also set:
  /// * `pose.position` is ignored in favor of latitude, longitude and
  ///   elevation.
  /// * `pose.orientation` is used in conjunction with heading:
  ///       Quaternion::fromEuler(0, 0, heading) * pose.orientation
  SphericalCoordinates spherical_coordinates = 11;
}
```

用于创建一个实体.使用`sdf`文件.

### WorldControl

```CPP
message WorldControl
{
  /// \brief Optional header data
  Header header        = 1;

  bool pause           = 2;
  bool step            = 3;
  uint32 multi_step    = 4;
  WorldReset reset     = 5;
  uint32 seed          = 6;

  // \brief A simulation time in the future to run to and then pause.
  Time run_to_sim_time = 7;
}
```

用于控制仿真环境.

### Wind

```CPP
message Wind
{
  /// \brief Optional header data
  Header header            = 1;

  Vector3d linear_velocity = 2;
  bool enable_wind         = 3;
}
```

风速

### IMU

```CPP
message IMU
{
  /// \brief Optional header data
  Header header                          = 1;
                                         
  string entity_name                     = 2;

  Quaternion orientation                 = 3;
  /// Row major about x, y, z
  Float_V orientation_covariance         = 4;

  Vector3d angular_velocity              = 5;
  /// Row major about x, y, z
  Float_V angular_velocity_covariance    = 6;

  Vector3d linear_acceleration           = 7;
  /// Row major about x, y, z
  Float_V linear_acceleration_covariance = 8;
}
```

IMU数据.
