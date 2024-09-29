# robot_state_publisher

参考文档

* [Robot State Publisher](https://github.com/ros/robot_state_publisher)

`robot_state_publisher`发布机器人信息给`tf2`,在启动阶段，`robot_state_publisher`读取`URDF`，获取运动学信息.它之后会订阅`joint_states`主题来获取单独节点的状态.这些关节状态用于进行运动学解算，将得到的三维位姿发布到`tf2`。

`robot_state_publisher`处理两种不同的关节类型：固定关节和可移动关节.启动时发布到`/tf_static`主题,`QOS`是`transient_local`,因此以后的订阅总能获得最新的世界状态.每当`joint_states`消息中相应的关节更新时，可移动关节就会发布到常规`/tf`主题。

## 发布主题

* `robot_description`，类型为`std_msgs/msg/String`，这个主题主要是为了把目前正在处理的`URDF`文件名发布出去.
* `tf`,类型为`tf2_msgs/msg/TFMessage`,解算出的可移动关节三维位姿信息.
* `tf_static`,类型为`tf2_msgs/msg/TFMessage`,解算出的固定关节的三维位姿信息.

## 订阅主题

* `joint_states`,类型为`sensor_msgs/msg/JointState`,这个主题是`joint_state_publisher`发布的主题.

## 参数

* `robot_description`，字符串，表示要读取的`URDF`文件.
* `publish_frequency`,double类型，表示最大发布的频率。
* `ignore_timestamp`,布尔类型，是否无论关节信息的`timestamp`如何，都接受关节信息，或者仅当关节信息比上次`publish_frequency`要新。默认为`false`.
* `frame_prefix`,字符串，添加到已发布的`tf2`帧的前缀。默认为空字符串。
