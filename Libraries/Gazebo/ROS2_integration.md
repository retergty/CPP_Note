# ROS 2 integration

`Gazebo`可以集成到`ROS2`系统中，以下有三种集成类型，它们可以同时存在。

* 使用`ROS2`启动`Gazebo`
* 使用`ROS2`通过`ros_gz`桥与`Gazebo`通信，使用主题的方法与`Gazebo`通信。
* 使用`ROS`生成`Gazebo`模型：`Gazebo`世界可以包含在启动时加载的模型。但是，有时您需要在运行时生成模型。该任务可以使用`ROS 2`来执行。

使用以下的`ROS2`包

* [Github](https://github.com/gazebosim/ros_gz/tree/humble)

* [ros_gz](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz)提供所有其他包的元包
* [ros_gz_image](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_image)使用 image_transport 将图像从 Gazebo Transport 传输到 ROS 的单向传输桥
* [ros_gz_bridge](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_bridge)Gazebo Transport 和 ROS 之间的双向传输桥
* [ros_gz_sim](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_sim)方便使用 Gazebo Sim 和 ROS 的启动文件和可执行文件
* [ros_gz_sim_demos](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_sim_demos)Demos using the ROS-Gazebo integration
* [ros_gz_point_cloud](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_point_cloud)用于从 Gazebo Sim 模拟将点云发布到 ROS 的插件

## 使用ROS2与Gazebo通信

### ros_gz_bridge

`ros_gz_bridge`是一个包，提供了在`ROS2`和`Gazebo Transport`之间交换消息.它只支持有限预定义的类型,参考[官方README](https://github.com/gazebosim/ros_gz/tree/ros2/ros_gz_bridge)

在`ROS2`与`Gazebo`通信前，首先需要初始化一个桥梁，这个桥梁连接`ROS2`主题与`Gazebo`主题，这个桥梁可以是单向的也可以是双向的。

### 手动建立桥梁

表示主题桥梁的方法如下

```shell
/TOPIC@ROS_MSG@GZ_MSG
```

* `@`表示构建了双向桥梁。`[`表示从`Gazebo`到`ROS`,`]`表示从`ROS`到`Gazebo`.
* `/TOPIC`表示`Gazebo`内部的主题名，可以通过`gz topic -l`查看.
* `ROS_MSG`表示`ROS`这个主题的消息类型
* `GZ_MSG`表示`Gazebo`这个主题的消息类型。

```shell
ros2 run ros_gz_bridge parameter_bridge /scan@sensor_msgs/msg/LaserScan@gz.msgs.LaserScan
```

`ros_gz_bridge`的`parameter_bridge`可执行文件会构建一个桥梁,连接`Gazebo`的主题`/scan`，以及指定对应的`ROS2`消息类型为`sensor_msgs/msg/LaserScan`，对应的`Gazebo`消息类型为`gz.msgs.LaserScan`.同时，它还会创建`ROS2`主题`/scan`,这样，`ROS2`往这个主题发布消息或者接受消息就会传递到对应的`Gazebo`中去。

表示服务桥梁的方法如下

```shell
/SERVICE@ROS_MSG
```

* `/SERVICE`表示`Gazebo`内部的服务名，可以通过`gz service -l`查看.
* `ROS_MSG`表示`ROS`这个服务的消息类型

```shell
ros2 run ros_gz_bridge parameter_bridge /world/shapes/control@ros_gz_interfaces/srv/ControlWorld
```

和上述一致，`ros_gz_bridge`的`parameter_bridge`可执行文件会构建一个桥梁,连接`Gazebo`的服务`/world/shapes/control`，以及指定对应的`ROS2`消息类型为`ros_gz_interfaces/srv/ControlWorld`，.同时，它还会创建`ROS2`主题`/world/shapes/control`,这样，`ROS2`往这个服务发布消息就会传递到对应的`Gazebo`中去。

### 使用YAML文件建立桥梁

```yaml
- ros_topic_name: "ros_chatter"
  gz_topic_name: "gz_chatter"
  ros_type_name: "std_msgs/msg/String"
  gz_type_name: "gz.msgs.StringMsg"
  subscriber_queue: 5       # Default 10
  publisher_queue: 6        # Default 10
  lazy: true                # Default "false"
  direction: BIDIRECTIONAL  # Default "BIDIRECTIONAL" - Bridge both directions
                            # "GZ_TO_ROS" - Bridge Gz topic to ROS
                            # "ROS_TO_GZ" - Bridge ROS topic to Gz
```

使用`YAML`格式的文件建立桥梁。这个做法可以重新指定`ROS`主题的名字，避免了名称冲突。同时还可以指定一系列的参数。

这个创建了`ROS2`主题`/ros_chatter`。

```shell
ros2 launch ros_gz_bridge ros_gz_bridge.launch.py name:=ros_gz_bridge config_file:=<path_to_your_YAML_file>
```

使用`ros_gz_bridge`的`ros_gz_bridge.launch.py`就可以方便地使用这个`YAML`文件。

在不支持`ros_gz_bridge.launch.py`的`Gazebo`版本，可以使用

```shell
ros2 run ros_gz_bridge parameter_bridge --ros-args -p config_file:=$WORKSPACE/ros_gz/ros_gz_bridge/test/config/full.yaml
```

### 例子

#### Gazebo发送ROS2接收

```shell
# Shell A:
ros2 run ros_gz_bridge parameter_bridge /chatter@std_msgs/msg/String@gz.msgs.StringMsg
```

```shell
# Shell B:
ros2 topic echo /chatter
```

```shell
# Shell C:
gz topic -t /chatter -m gz.msgs.StringMsg -p 'data:"Hello"'
```

在三个终端分别输入以上的命令，便会在终端`B`得到`Gazebo`发送的`Hello`.

**注意!**,如果没有收到消息，很有可能是`Gazebo`和`ROS2`的版本不匹配。

#### Gazebo发送图像

```shell
# Shell A:
gz sim sensors_demo.sdf
```

```shell
# Shell B:
ros2 run ros_gz_bridge parameter_bridge /rgbd_camera/image@sensor_msgs/msg/Image@gz.msgs.Image
```

```shell
# Shell C:
ros2 run rqt_image_view rqt_image_view /rgbd_camera/image
```

在三个终端分别输入以上的命令，便会在终端`C`得到`Gazebo`发送的图像，注意在`Gazebo`里点击启动按钮。
