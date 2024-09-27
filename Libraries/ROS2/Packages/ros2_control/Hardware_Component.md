# Hardware Component

参考文档

* [Writing a Hardware Component](https://control.ros.org/humble/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)

硬件部分是一个使用`pluginlib`生成的动态库插件，它与硬件沟通，接收控制器的指令，实现`URDF`中定义的`interface`.

## 硬件部分基类

硬件部分基类所在的`ros2`包为`hardware_interface`.所在的头文件为`$interface_type$_interface.hpp`,名字为`hardware_interface::$InterfaceType$Interface`.其中，`$interface_type$`可以是`Actuator`,`Sensor`,`System`.

这个基类提供了一系列的虚函数，具体的硬件部分需要实现这些虚函数.

## 返回值

`$InterfaceType$Interface`类中的函数几乎都有类型为`CallbackReturn`的返回值，它是一个枚举类，有以下三个值

* `CallbackReturn::SUCCESS`函数成功执行
* `CallbackReturn::FAILURE`函数执行失败，但可以重新调用
* `CallbackReturn::ERROR`函数严重错误，必须由`on_error`函数处理错误。

## 硬件状态

硬件有如下的状态定义，在状态间转换时会调用`on_`类型的函数

* `UNCONFIGURED`,`on_init`,`on_cleanup`函数调用后会处于的状态.此时硬件已经初始化，但是还没有建立通信，所以没有`interface`可用.
* `INACTIVE`,`on_configure`,`on_deactivate`函数调用后会处于的状态.此时硬件硬件配置完成，已经建立了通信.`RM`可以调用`read`读取硬件状态，也可以调用`write`执行非运动的硬件命令.但是那些运动的硬件命令还不可用，这些`interface`是`HW_IF_POSITION`,`HW_IF_VELOCITY`,`HW_IF_ACCELERATION`,`HW_IF_EFFORT`.
* `FINALIZED`,`on_shutdown`函数调用后会处于的状态,此时硬件已经可以卸载或删除了.
* `ACTIVE`,`on_activate`函数调用后会处于的状态,硬件的已经完成上电启动,硬件可以移动.可以执行运动的硬件命令了.


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
LifecycleNodeInterface::CallbackReturn on_cleanup(const State &)
```

这是生命周期基类的虚函数，这个函数是`on_configure`的反义.

### on_activate

```CPP
hardware_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state)
```

这是生命周期基类的虚函数，通常用这个函数执行类似于给硬件启动的操作，比如上电等.执行完毕后，硬件已经可以工作.

### on_deactivate

```CPP
hardware_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state)
```

这是生命周期基类的虚函数，是`on_activate`的反义.

### on_shutdown

```CPP
hardware_interface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State & previous_state)
```

这是生命周期基类的虚函数,硬件在调用完毕这个函数后便关闭，可以被移除.

### on_error

```CPP
hardware_interface::CallbackReturn on_error(const rclcpp_lifecycle::State & previous_state)
```

这是生命周期基类的虚函数，如果`CallbackReturn`返回了`CallbackReturn::ERROR`，则需要这个函数进行处理.

### read

```CPP
hardware_interface::return_type read(const rclcpp::Time & time, const rclcpp::Duration & period)
```

`RM`会调用这个函数读取硬件，这个类需要在这个函数里实际读取硬件并把读取到的值存储在之前由`export_state_interfaces`函数定义的内部变量中.

### write

```CPP
hardware_interface::return_type write(const rclcpp::Time & time, const rclcpp::Duration & period)
```

`RM`会调用这个函数写硬件，这个类需要根据`export_command_interfaces`定义的内部变量执行特定的硬件命令.

