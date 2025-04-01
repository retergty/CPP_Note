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

它公有继承`OutputModuleInterface`类，实际上是一个独立的`workitem`，具有独有的`Run`函数。

```CPP
MixingOutput _mixing_output{"SIM_GZ_EC", MAX_ACTUATORS, *this, MixingOutput::SchedulingPolicy::Auto, false, false};
```

这个成员对象的类型为`MixingOutput`,会自动读取指定前缀的参数，从而知晓它要读取的uORB消息类型与电机个数,并在`actuator_motors`消息到来时把`GZMixingInterfaceESC`挂到指定的工作队列中去.

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

```text
param set-default SIM_GZ_EC_FUNC1 101
param set-default SIM_GZ_EC_FUNC2 102
param set-default SIM_GZ_EC_FUNC3 103
param set-default SIM_GZ_EC_FUNC4 104

param set-default SIM_GZ_EC_MIN1 150
param set-default SIM_GZ_EC_MIN2 150
param set-default SIM_GZ_EC_MIN3 150
param set-default SIM_GZ_EC_MIN4 150

param set-default SIM_GZ_EC_MAX1 1000
param set-default SIM_GZ_EC_MAX2 1000
param set-default SIM_GZ_EC_MAX3 1000
param set-default SIM_GZ_EC_MAX4 1000
```

设置了`MixOutput`会读取的参数.这些参数包含选择的输出类型，输出最大值，最小值.

## `MixingOutput`类

```CPP
class MixingOutput : public ModuleParams
```

`MixingOutput`类就是用于管理输出的类。它读取参数所设定的类型，比如电机等，多态分配指定的函数接受uORB消息，处理并传递给实际的输出控制类.

### 构造函数

```CPP
/**
 * Constructor
 * @param param_prefix for min/max/etc. params, e.g. "PWM_MAIN". This needs to match 'param_prefix' in the module.yaml
 * @param max_num_outputs maximum number of supported outputs
 * @param interface Parent module for scheduling, parameter updates and callbacks
 * @param scheduling_policy
 * @param support_esc_calibration true if the output module supports ESC calibration via max, then min setting
 * @param ramp_up true if motor ramp up from disarmed to min upon arming is wanted
 */
MixingOutput(const char *param_prefix, uint8_t max_num_outputs, OutputModuleInterface &interface,
        SchedulingPolicy scheduling_policy,
        bool support_esc_calibration, bool ramp_up = true, const uint8_t instance_start = 1);
```

* `param_prefix`表示要读取的参数的前缀.
* `max_num_outputs`表示支持最多的输出.
* `interface`为包含这个类的类实例，用于在接受到消息时把这个类插入到指定的工作队列中去.

### 参数读取与更新

```CPP
void MixingOutput::initParamHandles(const uint8_t instance_start)
{
 char param_name[17];

 for (unsigned i = 0; i < _max_num_outputs; ++i) {
  snprintf(param_name, sizeof(param_name), "%s_%s%d", _param_prefix, "FUNC", i + instance_start);
  _param_handles[i].function = param_find(param_name);
  snprintf(param_name, sizeof(param_name), "%s_%s%d", _param_prefix, "DIS", i + instance_start);
  _param_handles[i].disarmed = param_find(param_name);
  snprintf(param_name, sizeof(param_name), "%s_%s%d", _param_prefix, "MIN", i + instance_start);
  _param_handles[i].min = param_find(param_name);
  snprintf(param_name, sizeof(param_name), "%s_%s%d", _param_prefix, "MAX", i + instance_start);
  _param_handles[i].max = param_find(param_name);
  snprintf(param_name, sizeof(param_name), "%s_%s%d", _param_prefix, "FAIL", i + instance_start);
  _param_handles[i].failsafe = param_find(param_name);
 }

 snprintf(param_name, sizeof(param_name), "%s_%s", _param_prefix, "REV");
 _param_handle_rev_range = param_find(param_name);
}
```

这个函数初始化参数句柄，用于之后的参数读取.

* `function`表示要使用的输出方法
* `min`表示输出最小值
* `max`表示输出最大值
* `_param_handle_rev_range`是一个`bit flag`表示是否翻转最大最小值.

```CPP
void MixingOutput::updateParams()
{
 ModuleParams::updateParams();

 bool function_changed = false;

 for (unsigned i = 0; i < _max_num_outputs; i++) {
  int32_t val;

  if (_param_handles[i].function != PARAM_INVALID && param_get(_param_handles[i].function, &val) == 0) {
   if (val != (int32_t)_function_assignment[i]) {
    function_changed = true;
   }

   // we set _function_assignment[i] later to ensure _functions[i] is updated at the same time
  }

  if (_param_handles[i].disarmed != PARAM_INVALID && param_get(_param_handles[i].disarmed, &val) == 0) {
   _disarmed_value[i] = val;
  }

  if (_param_handles[i].min != PARAM_INVALID && param_get(_param_handles[i].min, &val) == 0) {
   _min_value[i] = val;
  }

  if (_param_handles[i].max != PARAM_INVALID && param_get(_param_handles[i].max, &val) == 0) {
   _max_value[i] = val;
  }

  if (_min_value[i] > _max_value[i]) {
   uint16_t tmp = _min_value[i];
   _min_value[i] = _max_value[i];
   _max_value[i] = tmp;
  }

  if (_param_handles[i].failsafe != PARAM_INVALID && param_get(_param_handles[i].failsafe, &val) == 0) {
   _failsafe_value[i] = val;
  }
 }

 _reverse_output_mask = 0;
 int32_t rev_range_param;

 if (_param_handle_rev_range != PARAM_INVALID && param_get(_param_handle_rev_range, &rev_range_param) == 0) {
  _reverse_output_mask = rev_range_param;
 }

 if (function_changed) {
  _need_function_update = true;
 }
}
```

实际更新参数.

### `updateSubscriptions`函数

```CPP
bool MixingOutput::updateSubscriptions(bool allow_wq_switch)
{
 if (!_need_function_update || _armed.armed) {
  return false;
 }

 // must be locked to potentially change WorkQueue
 lock();

 _has_backup_schedule = false;

 if (_scheduling_policy == SchedulingPolicy::Auto) {
  // first clear everything
  unregister();
  _interface.ScheduleClear();

  bool switch_requested = false;

  // potentially switch work queue if we run motor outputs
  for (unsigned i = 0; i < _max_num_outputs; i++) {
   // read function directly from param, as _function_assignment[i] is updated later
   int32_t function;

   if (_param_handles[i].function != PARAM_INVALID && param_get(_param_handles[i].function, &function) == 0) {
    if (function >= (int32_t)OutputFunction::Motor1 && function <= (int32_t)OutputFunction::MotorMax) {
     switch_requested = true;
    }
   }
  }

  if (allow_wq_switch && !_wq_switched && switch_requested) {
   if (_interface.ChangeWorkQueue(px4::wq_configurations::rate_ctrl)) {
    // let the new WQ handle the subscribe update
    _wq_switched = true;
    _interface.ScheduleNow();
    unlock();
    return false;
   }
  }
 }

 // Now update the functions
 PX4_DEBUG("updating functions");

 cleanupFunctions();

 const FunctionProviderBase::Context context{_interface, _param_thr_mdl_fac.reference()};
 int provider_indexes[MAX_ACTUATORS] {};
 int next_provider = 0;
 int subscription_callback_provider_index = INT_MAX;
 bool all_disabled = true;

 for (int i = 0; i < _max_num_outputs; ++i) {
  int32_t val;

  if (_param_handles[i].function != PARAM_INVALID && param_get(_param_handles[i].function, &val) == 0) {
   _function_assignment[i] = (OutputFunction)val;

  } else {
   _function_assignment[i] = OutputFunction::Disabled;
  }

  for (int p = 0; p < (int)(sizeof(all_function_providers) / sizeof(all_function_providers[0])); ++p) {
   if (_function_assignment[i] >= all_function_providers[p].min_func &&
       _function_assignment[i] <= all_function_providers[p].max_func) {
    all_disabled = false;
    int found_index = -1;

    for (int existing = 0; existing < next_provider; ++existing) {
     if (provider_indexes[existing] == p) {
      found_index = existing;
      break;
     }
    }

    if (found_index >= 0) {
     _functions[i] = _function_allocated[found_index];

    } else {
     _function_allocated[next_provider] = all_function_providers[p].constructor(context);

     if (_function_allocated[next_provider]) {
      _functions[i] = _function_allocated[next_provider];
      provider_indexes[next_provider++] = p;

      // lowest provider takes precedence for scheduling
      if (p < subscription_callback_provider_index && _functions[i]->subscriptionCallback()) {
       subscription_callback_provider_index = p;
       _subscription_callback = _functions[i]->subscriptionCallback();
      }

     } else {
      PX4_ERR("function alloc failed");
     }
    }

    break;
   }
  }
 }

 hrt_abstime fixed_rate_scheduling_interval = 4_ms; // schedule at 250Hz

 if (_max_topic_update_interval_us > fixed_rate_scheduling_interval) {
  fixed_rate_scheduling_interval = _max_topic_update_interval_us;
 }

 if (_scheduling_policy == SchedulingPolicy::Auto) {
  if (_subscription_callback) {
   if (_subscription_callback->registerCallback()) {
    PX4_DEBUG("Scheduling via callback");
    _has_backup_schedule = true;
    _interface.ScheduleDelayed(50_ms);

   } else {
    PX4_ERR("registerCallback failed, scheduling at fixed rate");
    _interface.ScheduleOnInterval(fixed_rate_scheduling_interval);
   }

  } else if (all_disabled) {
   _interface.ScheduleOnInterval(_lowrate_schedule_interval);
   PX4_DEBUG("Scheduling at low rate");

  } else {
   _interface.ScheduleOnInterval(fixed_rate_scheduling_interval);
   PX4_DEBUG("Scheduling at fixed rate");
  }
 }

 setMaxTopicUpdateRate(_max_topic_update_interval_us);
 _need_function_update = false;

 _actuator_test.reset();

 unlock();

 _interface.mixerChanged();

 return true;
}
```

这个函数更新选择的输出函数.给每个输出都按照参数选择输出函数，如果是第一次实例化这个输出函数，那么还需要构建。

```CPP
_function_assignment[i] = (OutputFunction)val;
```

`_function_assignment`存储了第`i`个输出对应与它所独有的`val`，比如第一个`motor`的值是`101`,第二个`motor`是`102`，这些值通过参数传递的，用来唯一表示了这个电机.

```CPP
static const FunctionProvider all_function_providers[] = {
 // Providers higher up take precedence for subscription callback in case there are multiple
 {OutputFunction::Constant_Min, &FunctionConstantMin::allocate},
 {OutputFunction::Constant_Max, &FunctionConstantMax::allocate},
 {OutputFunction::Motor1, OutputFunction::MotorMax, &FunctionMotors::allocate},
 {OutputFunction::Servo1, OutputFunction::ServoMax, &FunctionServos::allocate},
 {OutputFunction::Peripheral_via_Actuator_Set1, OutputFunction::Peripheral_via_Actuator_Set6, &FunctionActuatorSet::allocate},
 {OutputFunction::Landing_Gear, &FunctionLandingGear::allocate},
 {OutputFunction::Landing_Gear_Wheel, &FunctionLandingGearWheel::allocate},
 {OutputFunction::Parachute, &FunctionParachute::allocate},
 {OutputFunction::Gripper, &FunctionGripper::allocate},
 {OutputFunction::RC_Roll, OutputFunction::RC_AUXMax, &FunctionManualRC::allocate},
 {OutputFunction::Gimbal_Roll, OutputFunction::Gimbal_Yaw, &FunctionGimbal::allocate},
};
```

`all_function_providers`存储了所有的可以提供的输出函数.

### `update`函数

```CPP
bool MixingOutput::update()
{
 // check arming state
 if (_armed_sub.update(&_armed)) {
  _armed.in_esc_calibration_mode &= _support_esc_calibration;

  if (_ignore_lockdown) {
   _armed.lockdown = false;
  }

  /* Update the armed status and check that we're not locked down.
   * We also need to arm throttle for the ESC calibration. */
  _throttle_armed = (_armed.armed && !_armed.lockdown) || _armed.in_esc_calibration_mode;
 }

 // only used for sitl with lockstep
 bool has_updates = _subscription_callback && _subscription_callback->updated();

 // update topics
 for (int i = 0; i < MAX_ACTUATORS && _function_allocated[i]; ++i) {
  _function_allocated[i]->update();
 }

 if (_has_backup_schedule) {
  _interface.ScheduleDelayed(50_ms);
 }

 // check for actuator test
 _actuator_test.update(_max_num_outputs, _param_thr_mdl_fac.get());

 // get output values
 float outputs[MAX_ACTUATORS];
 bool all_disabled = true;
 _reversible_mask = 0;

 for (int i = 0; i < _max_num_outputs; ++i) {
  if (_functions[i]) {
   all_disabled = false;

   if (_armed.armed || (_armed.prearmed && _functions[i]->allowPrearmControl())) {
    outputs[i] = _functions[i]->value(_function_assignment[i]);

   } else {
    outputs[i] = NAN;
   }

   _reversible_mask |= (uint32_t)_functions[i]->reversible(_function_assignment[i]) << i;

  } else {
   outputs[i] = NAN;
  }
 }

 // Send output if any function mapped or one last disabling sample
 if (!all_disabled || !_was_all_disabled) {
  if (!_armed.armed && !_armed.manual_lockdown) {
   _actuator_test.overrideValues(outputs, _max_num_outputs);
  }

  limitAndUpdateOutputs(outputs, has_updates);
 }

 _was_all_disabled = all_disabled;

 return true;
}

void
MixingOutput::limitAndUpdateOutputs(float outputs[MAX_ACTUATORS], bool has_updates)
{
 bool stop_motors = !_throttle_armed && !_actuator_test.inTestMode();

 if (_armed.lockdown || _armed.manual_lockdown) {
  // overwrite outputs in case of lockdown with disarmed values
  for (size_t i = 0; i < _max_num_outputs; i++) {
   _current_output_value[i] = _disarmed_value[i];
  }

  stop_motors = true;

 } else if (_armed.force_failsafe) {
  // overwrite outputs in case of force_failsafe with _failsafe_value values
  for (size_t i = 0; i < _max_num_outputs; i++) {
   _current_output_value[i] = actualFailsafeValue(i);
  }

 } else {
  // the output limit call takes care of out of band errors, NaN and constrains
  output_limit_calc(_throttle_armed || _actuator_test.inTestMode(), _max_num_outputs, outputs);
 }

 // We must calibrate the PWM and Oneshot ESCs to a consistent range of 1000-2000us (gets mapped to 125-250us for Oneshot)
 // Doing so makes calibrations consistent among different configurations and hence PWM minimum and maximum have a consistent effect
 // hence the defaults for these parameters also make most setups work out of the box
 if (_armed.in_esc_calibration_mode) {
  static constexpr uint16_t PWM_CALIBRATION_LOW = 1000;
  static constexpr uint16_t PWM_CALIBRATION_HIGH = 2000;

  for (int i = 0; i < _max_num_outputs; i++) {
   if (_current_output_value[i] == _min_value[i]) {
    _current_output_value[i] = PWM_CALIBRATION_LOW;
   }

   if (_current_output_value[i] == _max_value[i]) {
    _current_output_value[i] = PWM_CALIBRATION_HIGH;
   }
  }
 }

 /* now return the outputs to the driver */
 if (_interface.updateOutputs(stop_motors, _current_output_value, _max_num_outputs, has_updates)) {
  actuator_outputs_s actuator_outputs{};
  setAndPublishActuatorOutputs(_max_num_outputs, actuator_outputs);

  updateLatencyPerfCounter(actuator_outputs);
 }
}
```

`update`函数更新从`_functions`读取来的要设置的输出值，填充`outputs`数组，调用`GZMixingInterfaceESC::updateOutputs`实际发送给硬件.

## `FunctionMotors`类

```CPP
class FunctionMotors : public FunctionProviderBase
```

`FunctionMotors`类订阅控制分配发布来的标准化的`[0,1]`或`[-1,1]`的`actuator_motors`的消息，自动把模块挂载到工作队列中.将收到的消息进行处理，映射到`[min,max]`中

```CPP
void update() override
{
 if (_topic.update(&_data)) {
  updateValues(_data.reversible_flags, _thrust_factor, _data.control, actuator_motors_s::NUM_CONTROLS);
 }
}

float value(OutputFunction func) override { return _data.control[(int)func - (int)OutputFunction::Motor1]; }
```

`update`成员函数接收并处理`actuator_motors`.

`value`成员函数返回指定的电机当前的设定值.

```CPP
static inline void updateValues(uint32_t reversible, float thrust_factor, float *values, int num_values)
{
 if (thrust_factor > 0.f && thrust_factor <= 1.f) {
  // thrust factor
  //  rel_thrust = factor * x^2 + (1-factor) * x,
  const float a = thrust_factor;
  const float b = (1.f - thrust_factor);

  // don't recompute for all values (ax^2+bx+c=0)
  const float tmp1 = b / (2.f * a);
  const float tmp2 = b * b / (4.f * a * a);

  for (int i = 0; i < num_values; ++i) {
   float control = values[i];

   if (control > 0.f) {
    values[i] = -tmp1 + sqrtf(tmp2 + (control / a));

   } else if (control < -0.f) {
    values[i] =  tmp1 - sqrtf(tmp2 - (control / a));

   } else {
    values[i] = 0.f;
   }
  }
 }

 for (int i = 0; i < num_values; ++i) {
  if ((reversible & (1u << i)) == 0) {
   if (values[i] < -FLT_EPSILON) {
    values[i] = NAN;

   } else {
    // remap from [0, 1] to [-1, 1]
    values[i] = values[i] * 2.f - 1.f;
   }
  }
 }
}
```

`updateValues`具体实现处理消息的逻辑，主要是应用电机非线性点，同时如果电机支持反转，那么把值从`[0,1]`映射到`[-1,1]`.
