# ros2 control examples

本文总结`ros2_control_demos`包里的例子的特点，方便日后查找.

## example_5

* 将机器人的`hardware components`与传感器的`hardware components`分开.机器人的硬件部分类为`ros2_control_demo_example_5/RRBotSystemPositionOnlyHardware`,传感器的硬件部分类为`ros2_control_demo_example_5/ExternalRRBotForceTorqueSensorHardware`.

## example_6

* 将机器人的`joint`的`hardware components`分开，每个`hardware components`只有一个`joint`.而且每个`hardware components`的类型还是相同的.

## example_7

* 是一个6自由度机器人的完全例子.从`URDF`到`hardware components`再到`controller`的例子.
* 首次使用了`kdl`库进行正逆运动学之间的转换.

## example_8

* 使用了`transmission`模拟了关节与执行器间的简单交互，比如齿轮减速比间的关系.控制器输入目标关节空间位置，自动转换为执行器的目标空间位置.
