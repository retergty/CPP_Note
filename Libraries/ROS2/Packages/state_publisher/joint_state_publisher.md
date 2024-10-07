# joint_state_publisher

`joint_state_publisher`包接受`URDF`描述的机器人构型信息，发布`sensor_msgs/JointState`类型的信息。

参考文档

* [Joint State Publisher](https://github.com/ros/joint_state_publisher/tree/noetic-devel/joint_state_publisher)

## 发布的主题

* `/joint_states`,类型为`sensor_msgs/msg/JointState`,描述了系统所有可移动关节的状态.

## 订阅的主题

* 由`sources_list`指定了主题名,类型为`sensor_msgs/msg/JointState`.

## 参数

* `robot_description`,字符串，指明了要读取的`URDF`信息.
* `rate`,整数，表示`/joint_states`主题发布的频率，默认为`10`.
* `publish_default_positions`,布尔值,是否给每个关节发布默认的位置，默认为真.
* `publish_default_velocities`,布尔值，是否给每个关节发布默认的速度，默认为真.
* `publish_default_efforts`,布尔值,是否给每个关节发布默认的力矩，默认为真.
* `use_mimic_tags`,布尔值，是否考虑`URDF`里的`<mimic>`标签.
* `use_smallest_joint_limits`，布尔值,是否考虑`URDF`里的`<safety_controller>`标签.
* `source_list`,字符串数组，要订阅的主题名.
* `zeros` 关节名称到关节初始值的字典。默认为空字典，在这种情况下，`0.0`被假定为所有关节的零值.

## joint_state_publisher_gui

`joint_state_publisher_gui`包提供了一个`gui`工具，可以使用`gui`工具发布与获取给定`URDF`的各个关节状态。
