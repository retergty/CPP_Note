# mixer_module

`mixer_module`不是一个模块，而是嵌入在输出控制模块里的一个库，实现的功能如下

* 用于实际与硬件资源交流的输出控制类中，作为一个成员，实现通用的功能。
* 接受指定的输出类型的uORB消息，并将之处理,调用实际输出控制类的回调函数.
* 使用参数系统，使得不同输出类型的硬件都可以在同一个类中使用，大幅减小了控制粒度。

本文以`simulation/gz_bridge`里用于`gazebo`仿真的输出控制以及`gz_x500`经典四旋翼无人机模型为例，讲解`px4`硬件输出的设计逻辑.

## `GZBridge`类

```CPP
class GZBridge : public ModuleBase<GZBridge>, public ModuleParams, public px4::ScheduledWorkItem
```

`GZBridge`类处理`gazebo`仿真的事务，比如从`gazebo`中接受`imu`消息,接受控制分配的电机转速设定并发送给`gazebo`.

```CPP
GZMixingInterfaceESC   _mixing_interface_esc{_node, _node_mutex};
GZMixingInterfaceServo _mixing_interface_servo{_node, _node_mutex};
GZMixingInterfaceWheel _mixing_interface_wheel{_node, _node_mutex};
```

这三个类实际上接受控制分配发来的`actuator_motors`或者是`actuator_servos`,并转换发送给`gazebo`.

## `GZMixingInterfaceESC`类

```CPP
class GZMixingInterfaceESC : public OutputModuleInterface
```

它实际上是一个独立的`workitem`，具有独有的`Run`函数。

```CPP
MixingOutput _mixing_output{"SIM_GZ_EC", MAX_ACTUATORS, *this, MixingOutput::SchedulingPolicy::Auto, false, false};
```

这个成员对象会自动读取指定前缀的参数，从而知晓它要读取的uORB消息类型与电机个数,并在`actuator_motors`消息到来时把`GZMixingInterfaceESC`挂到指定的工作队列中去.

### `Run`成员函数

```CPP
void GZMixingInterfaceESC::Run()
{
  pthread_mutex_lock(&_node_mutex);
  _mixing_output.update();
  _mixing_output.updateSubscriptions(false);
  pthread_mutex_unlock(&_node_mutex);
}
```

`Run`函数会在`actuator_motors`消息发生时自动运行，它只是调用了`_mixing_output`的成员函数便可以自动地接受`actuator_motors`消息并发送`gazebo`的控制消息.

### `updateOutputs`成员函数

```CPP
bool GZMixingInterfaceESC::updateOutputs(bool stop_motors, uint16_t outputs[MAX_ACTUATORS], unsigned num_outputs,
    unsigned num_control_groups_updated)
{
  unsigned active_output_count = 0;

  for (unsigned i = 0; i < num_outputs; i++) {
    if (_mixing_output.isFunctionSet(i)) {
      active_output_count++;

    } else {
      break;
    }
  }

  if (active_output_count > 0) {
    gz::msgs::Actuators rotor_velocity_message;
    rotor_velocity_message.mutable_velocity()->Resize(active_output_count, 0);

    for (unsigned i = 0; i < active_output_count; i++) {
      rotor_velocity_message.set_velocity(i, outputs[i]);
    }

    if (_actuators_pub.Valid()) {
      return _actuators_pub.Publish(rotor_velocity_message);
    }
  }

  return false;
}
```

`updateOutputs`会在`_mixing_output.update()`里被调用，接受控制电机输出参数，发送`gazebo`的控制消息。

### `4001_gz_x500`构型文件

```

## `MixingOutput`类

```CPP
class MixingOutput : public ModuleParams
```

`MixingOutput`类就是用于管理输出的类。它读取参数所设定的类型，比如电机等，多态分配指定的函数接受uORB消息，处理并传递给实际的输出控制类.