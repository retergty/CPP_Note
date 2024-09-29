# Hardware Component

参考文档

* [Writing a Hardware Component](https://control.ros.org/humble/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)

硬件部分是一个使用`pluginlib`生成的动态库插件，它与硬件沟通，接收控制器的指令，实现`URDF`中定义的`interface`.

## 硬件部分基类

硬件部分基类所在的`ros2`包为`hardware_interface`.所在的头文件为`$interface_type$_interface.hpp`,名字为`$InterfaceType$Interface`处于名称空间`hardware_interface`中.其中，`$interface_type$`可以是`Actuator`,`Sensor`,`System`.

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

## 常见成员变量

```CPP
HardwareInfo info_;
```

就保存了硬件信息，由基类的`on_init`函数赋值

```CPP
virtual CallbackReturn on_init(const HardwareInfo & hardware_info)
{
  info_ = hardware_info;
  return CallbackReturn::SUCCESS;
};
```

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

## HardwareInfo类

`HardwareInfo`是定义在`hardware_interface`名称空间里的类，它是读取了`URDF`文件后所保存的信息.

```CPP
/// This structure stores information about hardware defined in a robot's URDF.
struct HardwareInfo
{
  /// Name of the hardware.
  std::string name;
  /// Type of the hardware: actuator, sensor or system.
  std::string type;
  /// Class of the hardware that will be dynamically loaded.
  std::string hardware_class_type;
  /// (Optional) Key-value pairs for hardware parameters.
  std::unordered_map<std::string, std::string> hardware_parameters;
  /**
   * Map of joints provided by the hardware where the key is the joint name.
   * Required for Actuator and System Hardware.
   */
  std::vector<ComponentInfo> joints;
  /**
   * Map of sensors provided by the hardware where the key is the joint or link name.
   * Required for Sensor and optional for System Hardware.
   */
  std::vector<ComponentInfo> sensors;
  /**
   * Map of GPIO provided by the hardware where the key is a descriptive name of the GPIO.
   * Optional for any hardware components.
   */
  std::vector<ComponentInfo> gpios;
  /**
   * Map of transmissions to calculate ration between joints and physical actuators.
   * Optional for Actuator and System Hardware.
   */
  std::vector<TransmissionInfo> transmissions;
  /**
   * The XML contents prior to parsing
   */
  std::string original_xml;
};
```

可以看到，之前在`URDF`里定义的标签都出现在了这里.

```CPP
struct ComponentInfo
{
  /// Name of the component.
  std::string name;
  /// Type of the component: sensor, joint, or GPIO.
  std::string type;
  /**
   * Name of the command interfaces that can be set, e.g. "position", "velocity", etc.
   * Used by joints and GPIOs.
   */
  std::vector<InterfaceInfo> command_interfaces;
  /**
   * Name of the state interfaces that can be read, e.g. "position", "velocity", etc.
   * Used by joints, sensors and GPIOs.
   */
  std::vector<InterfaceInfo> state_interfaces;
  /// (Optional) Key-value pairs of component parameters, e.g. min/max values or serial number.
  std::unordered_map<std::string, std::string> parameters;
};
```

```CPP
struct InterfaceInfo
{
  /**
   * Name of the command interfaces that can be set, e.g. "position", "velocity", etc.
   * Used by joints and GPIOs.
   */
  std::string name;
  /// (Optional) Minimal allowed values of the interface.
  std::string min;
  /// (Optional) Maximal allowed values of the interface.
  std::string max;
  /// (Optional) Initial value of the interface.
  std::string initial_value;
  /// (Optional) The datatype of the interface, e.g. "bool", "int". Used by GPIOs.
  std::string data_type;
  /// (Optional) If the handle is an array, the size of the array. Used by GPIOs.
  int size;
};
```

用户自定义的参数会出现在`parameters`里.

## StateInterface类

`StateInterface`是定义在`hardware_interface`名称空间里的类，它是`export_state_interfaces`会导出的类型.

```CPP
class StateInterface : public ReadOnlyHandle
{
public:
  StateInterface(const StateInterface & other) = default;

  StateInterface(StateInterface && other) = default;

  using ReadOnlyHandle::ReadOnlyHandle;
};
```

```CPP
/// A handle used to get and set a value on a given interface.
class ReadOnlyHandle
{
public:
  ReadOnlyHandle(
    const std::string & prefix_name, const std::string & interface_name,
    double * value_ptr = nullptr)
  : prefix_name_(prefix_name), interface_name_(interface_name), value_ptr_(value_ptr)
  {
  }

  explicit ReadOnlyHandle(const std::string & interface_name)
  : interface_name_(interface_name), value_ptr_(nullptr)
  {
  }

  explicit ReadOnlyHandle(const char * interface_name)
  : interface_name_(interface_name), value_ptr_(nullptr)
  {
  }

  ReadOnlyHandle(const ReadOnlyHandle & other) = default;

  ReadOnlyHandle(ReadOnlyHandle && other) = default;

  ReadOnlyHandle & operator=(const ReadOnlyHandle & other) = default;

  ReadOnlyHandle & operator=(ReadOnlyHandle && other) = default;

  virtual ~ReadOnlyHandle() = default;

  /// Returns true if handle references a value.
  inline operator bool() const { return value_ptr_ != nullptr; }

  const std::string get_name() const { return prefix_name_ + "/" + interface_name_; }

  const std::string & get_interface_name() const { return interface_name_; }

  [[deprecated(
    "Replaced by get_name method, which is semantically more correct")]] const std::string
  get_full_name() const
  {
    return get_name();
  }

  const std::string & get_prefix_name() const { return prefix_name_; }

  double get_value() const
  {
    THROW_ON_NULLPTR(value_ptr_);
    return *value_ptr_;
  }

protected:
  std::string prefix_name_;
  std::string interface_name_;
  double * value_ptr_;
};
```

最关键的就是它的成员变量

* `prefix_name_`表示前缀名,通常是所在关节的关节名.
* `interface_name_`表示这个`interface`的名字
* `value_ptr_`这是一个指针,指向实际的值，指向类内部保存的值.

## CommandInterface类

`CommandInterface`是定义在`hardware_interface`名称空间里的类，它是`export_command_interfaces`会导出的类型.

```CPP
class CommandInterface : public ReadWriteHandle
{
public:
  /// CommandInterface copy constructor is actively deleted.
  /**
   * Command interfaces are having a unique ownership and thus
   * can't be copied in order to avoid simultaneous writes to
   * the same resource.
   */
  CommandInterface(const CommandInterface & other) = delete;

  CommandInterface(CommandInterface && other) = default;

  using ReadWriteHandle::ReadWriteHandle;
};
```

```CPP
class ReadWriteHandle : public ReadOnlyHandle
{
public:
  ReadWriteHandle(
    const std::string & prefix_name, const std::string & interface_name,
    double * value_ptr = nullptr)
  : ReadOnlyHandle(prefix_name, interface_name, value_ptr)
  {
  }

  explicit ReadWriteHandle(const std::string & interface_name) : ReadOnlyHandle(interface_name) {}

  explicit ReadWriteHandle(const char * interface_name) : ReadOnlyHandle(interface_name) {}

  ReadWriteHandle(const ReadWriteHandle & other) = default;

  ReadWriteHandle(ReadWriteHandle && other) = default;

  ReadWriteHandle & operator=(const ReadWriteHandle & other) = default;

  ReadWriteHandle & operator=(ReadWriteHandle && other) = default;

  virtual ~ReadWriteHandle() = default;

  void set_value(double value)
  {
    THROW_ON_NULLPTR(this->value_ptr_);
    *this->value_ptr_ = value;
  }
};
```

其实与`StateInterface`不同的是，它有`set_value`，可以向`value_ptr`写数据.
