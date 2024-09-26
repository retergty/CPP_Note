# Hardware Component

参考文档

* [Writing a Hardware Component](https://control.ros.org/humble/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)

硬件部分是一个使用`pluginlib`生成的动态库插件，它与硬件沟通，接收控制器的指令，实现`URDF`中定义的`interface`.

## 硬件部分基类

硬件部分基类所在的`ros2`包为`hardware_interface`.所在的头文件为`$interface_type$_interface.hpp`,名字为`hardware_interface::$InterfaceType$Interface`.其中，`$interface_type$`可以是`Actuator`,`Sensor`,`System`.

这个基类提供了一系列的虚函数，具体的硬件部分需要实现这些虚函数.

## 常见虚函数

### on_init

```CPP
virtual CallbackReturn on_init(const HardwareInfo & hardware_info);
```

会在硬件部分初始化时调用,使用从`URDF`传递来的数据初始化硬件部分.通常派生类也需要调用基类的`on_init`.

由于不允许带有参数的构造函数，所以通常这个函数用来初始化所有的成员变量与类外资源.

* `hardware_info`从`URDF`中读取到的数据结构体.

### export_state_interfaces,export_command_interfaces

```CPP
virtual std::vector<StateInterface> export_state_interfaces()
virtual std::vector<CommandInterface> export_command_interfaces()
```

导出`state interface`与`command interface`,将它们的所有权传递给`Resource Manager`.

如果返回空`vector`，那么会调用默认的函数导出`interface`.

```CPP
std::vector<CommandInterface::SharedPtr> on_export_command_interfaces();
std::vector<StateInterface::SharedPtr> on_export_state_interfaces();
```

### on_configure

```CPP
hardware_interface::CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state)
```

这是生命周期基类的虚函数，通常用这个函数配置与硬件的通信，设置所有与硬件有关的内容.以便于这个硬件可以被启动(activated).

### on_cleanup

```CPP
<LifecycleNodeInterface::CallbackReturn(const State &
```


