# Hardware Component

参考文档

* [Writing a Hardware Component](https://control.ros.org/humble/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)

硬件部分是一个使用`pluginlib`生成的动态库插件，它与硬件沟通，接收控制器的指令，实现`URDF`中定义的`interface`.

## 硬件部分基类

硬件部分基类所在的`ros2`包为`hardware_interface`.所在的头文件为`$interface_type$_interface.hpp`,名字为`hardware_interface::$InterfaceType$Interface`