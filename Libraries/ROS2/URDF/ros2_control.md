# ros2 control

参考文档

* [ROS2 Control Components Architecture and URDF-Description](https://github.com/ros-controls/roadmap/blob/master/design_drafts/components_architecture_and_urdf_examples.md)

`ros2_control`包拓展了`URDF`文件，添加了`<ros2_control>`元素用来描述硬件部分。

一个机器人中可以有多个硬件部分，所以一个`URDF`中可以有多个`<ros2_control>`标签分别表示多个硬件部分。

## 指明一个硬件部分

```xml
<ros2_control name="RRBotSystemPositionOnly" type="system">
<ros2_control name="RRBotForceTorqueSensor2D" type="sensor">
<ros2_control name="RRBotModularJoint1" type="actuator">
```

* `name`指明了该硬件部分的名字。
* `type`指明了该硬件部分的类型。

### 声明硬件

```xml
<hardware>
  <plugin>ros2_control_demo_hardware/RRBotSystemPositionOnlyHardware</plugin>
  <param name="example_param_hw_start_duration_sec">2.0</param>
  <param name="example_param_hw_stop_duration_sec">3.0</param>
  <param name="example_param_hw_slowdown">2.0</param>
</hardware>
```

`<hardware>`声明了硬件，由于`ros2_control`是使用`plugin`管理硬件的，使用`<plugin>`指定了该硬件。

`<param>`为自定义的参数,取决于`plugin`能接受的参数。

### 声明关节

```xml
<joint name="joint1">
  <command_interface name="position">
    <param name="min">-1</param>
    <param name="max">1</param>
  </command_interface>
  <state_interface name="position"/>
</joint>
```

`<joint>`声明了硬件`plugin`要控制的关节，`name`是之前在`UDRF`其它位置出现过的`<joint>`的名字。

`<command_interface>`和`<state_interface>`声明了硬件`plugin`为这个`<joint>`提供的接口,`name`表示接口的类型，通常为`position`,`velocity`,`acceleration`,`effort`.

### 声明传感器

```xml
<sensor name="tcp_fts_sensor">
  <state_interface name="fx"/>
  <state_interface name="tz"/>
  <param name="frame_id">rrbot_tcp</param>
  <param name="fx_range">100</param>
  <param name="tz_range">15</param>
</sensor>
```

`<sensor>`声明了一个传感器，`name`是传感器的名称。

`<sensor>`可以集成到`system`中，也可以单独放在`sensor`中，取决于`plugin`能否接受传感器。

### 声明传动器

```xml
<transmission name="transmission1">
  <plugin>transmission_interface/RotationToLinerTansmission</plugin>
  <param name="joint_to_actuator">${1024/PI}</param>
</transmission>
```
