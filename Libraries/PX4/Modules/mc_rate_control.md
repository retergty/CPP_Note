# mc_rate_control

`mc_att_control`是多旋翼无人机的角加速度环控制器，它总是使能的，实现的功能如下

* 接收从`mc_att_control`发送来的`vehicle_rates_setpoint`
* 收到的`vehicle_rates_setpoint`进行`PID`控制，转化为期望机体坐标系三轴力矩`vehicle_torque_setpoint`与三轴推力`vehicle_thrust_setpoint`.
* 接收从`control_allocator`发布的`control_allocator_status`得知控制分配器是否饱和。
* 接收从底层驱动发布的`battery_status`得知电池状态。
* 发布`vehicle_torque_setpoint`,`vehicle_thrust_setpoint`给`control_allocator`

## 关键数据流

### 订阅

#### `vehicle_angular_velocity`

是这个模块的驱动订阅，表示无人机三轴的角速度与角加速度，由`EKF`模块发布。

#### `vehicle_rates_setpoint`

表示无人机角速度`setpoint`，由`mc_att_control`模块发布

#### `vehicle_control_mode`

表示无人机控制模式，对于旋翼无人机，这个模块只是使用`vehicle_control_mode.flag_armed`检测是否`arm`来决定是否重置积分器。

#### `control_allocator_status`

表示无人机控制分配的状态，这个模块使用这个信息决定是否执行器饱和，由`control_allocator`.

#### `vehicle_land_detected`

检测无人机是否落地，如果落地，需要处理积分器，防止其饱和，由`land_detector`发布.

#### `battery_status`

表示无人机电池当前的状态，如果电量过低，需要减少控制量，由底层驱动发布。

### 发布

#### `vehicle_torque_setpoint`

表示无人机力矩`setpoint`,这是一个标准化的力矩值，范围为`[-1,1]`,传递给`control_allocator`.

#### `vehicle_thrust_setpoint`

表示无人机推力`setpoint`,这是一个标准化的力矩值，范围为`[0,1]`,传递给`control_allocator`.

#### `actuator_controls_status_0`

表示无人机三轴力矩的功率，由`mc_autotune_attitude_control`使用，进行自动`PID`整定。

#### `vehicle_rates_setpoint`

在旋翼无人机下不会发布这个主题，因为`mc_att_control`总是使能的。

## `MulticopterRateControl`类

```CPP
class MulticopterRateControl : public ModuleBase<MulticopterRateControl>, public ModuleParams, public px4::WorkItem
```

`MulticopterRateControl`类是在`rate_ctrl`的工作队列中的模块。

### 关键成员变量

#### `_rate_control`

```CPP
RateControl _rate_control; ///< class for rate control calculations
```

核心的角速度`PID`控制器

#### `_thrust_setpoint`

```CPP
matrix::Vector3f _thrust_setpoint{};
```

机体坐标系三轴推力，由`vehicle_rates_setpoint`主题传递而来。

#### `_landed,_maybe_landed`

```CPP
bool _landed{true};
bool _maybe_landed{true};
```

表示无人机是否落地，由`vehicle_land_detected`主题传递而来。

## 代码分析

### 接收控制分配饱和信息

```CPP
if (_control_allocator_status_sub.update(&control_allocator_status)) {
  Vector<bool, 3> saturation_positive;
  Vector<bool, 3> saturation_negative;

  if (!control_allocator_status.torque_setpoint_achieved) {
    for (size_t i = 0; i < 3; i++) {
      if (control_allocator_status.unallocated_torque[i] > FLT_EPSILON) {
        saturation_positive(i) = true;

      } else if (control_allocator_status.unallocated_torque[i] < -FLT_EPSILON) {
        saturation_negative(i) = true;
      }
    }
  }

  // TODO: send the unallocated value directly for better anti-windup
  _rate_control.setSaturationStatus(saturation_positive, saturation_negative);
}
```

接收控制分配模块回传的信息，是否有未分配的力矩，未分配力矩的正负值，并设置给核心控制器。

```CPP
// prevent further positive control saturation
if (_control_allocator_saturation_positive(i)) {
  rate_error(i) = math::min(rate_error(i), 0.f);
}

// prevent further negative control saturation
if (_control_allocator_saturation_negative(i)) {
  rate_error(i) = math::max(rate_error(i), 0.f);
}
```

在`updateIntegral`中，会检测是否饱和，如果饱和就会降低避免积分器过度饱和。

### 根据电池状态降低`setpoint`

```CPP
// scale setpoints by battery status if enabled
if (_param_mc_bat_scale_en.get()) {
  if (_battery_status_sub.updated()) {
    battery_status_s battery_status;

    if (_battery_status_sub.copy(&battery_status) && battery_status.connected && battery_status.scale > 0.f) {
      _battery_status_scale = battery_status.scale;
    }
  }

  if (_battery_status_scale > 0.f) {
    for (int i = 0; i < 3; i++) {
      vehicle_thrust_setpoint.xyz[i] = math::constrain(vehicle_thrust_setpoint.xyz[i] * _battery_status_scale, -1.f, 1.f);
      vehicle_torque_setpoint.xyz[i] = math::constrain(vehicle_torque_setpoint.xyz[i] * _battery_status_scale, -1.f, 1.f);
    }
  }
}
```

接收`battery_status`,按照电池的`battery_status.scale`修正`setpoint`.

### 进行角速度PID控制

```CPP
const Vector3f att_control = _rate_control.update(rates, _rates_setpoint, angular_accel, dt, _maybe_landed || _landed);
```

```CPP
Vector3f RateControl::update(const Vector3f &rate, const Vector3f &rate_sp, const Vector3f &angular_accel,
           const float dt, const bool landed)
{
  // angular rates error
  Vector3f rate_error = rate_sp - rate;

  // PID control with feed forward
  const Vector3f torque = _gain_p.emult(rate_error) + _rate_int - _gain_d.emult(angular_accel) + _gain_ff.emult(rate_sp);

  // update integral only if we are not landed
  if (!landed) {
    updateIntegral(rate_error, dt);
  }

  return torque;
}
```

大致过程是经典的`PID`控制过程，但是在无人机降落时，不会更新积分器，防止饱和。注意，此时`rate_sp`具有单位`rad/s`,但是`torque`则变为标准化的`[-1,1]`的量。

$$
M_{normalize} = \frac{KM_{real}F_{hover}}{mg}
$$

其中， $K$ 是一个系数，如果想要加大力矩的控制量，可以增大，反之可以缩小，但是注意不要超过`[-1,1]`的区间。此外 $M_{noramlize}$ 是有单位的，单位为 $m$ .

最终返回机体坐标系三轴力矩`setpoint`.

```CPP
void RateControl::updateIntegral(Vector3f &rate_error, const float dt)
{
  for (int i = 0; i < 3; i++) {
    // prevent further positive control saturation
    if (_control_allocator_saturation_positive(i)) {
      rate_error(i) = math::min(rate_error(i), 0.f);
    }

    // prevent further negative control saturation
    if (_control_allocator_saturation_negative(i)) {
      rate_error(i) = math::max(rate_error(i), 0.f);
    }

    // I term factor: reduce the I gain with increasing rate error.
    // This counteracts a non-linear effect where the integral builds up quickly upon a large setpoint
    // change (noticeable in a bounce-back effect after a flip).
    // The formula leads to a gradual decrease w/o steps, while only affecting the cases where it should:
    // with the parameter set to 400 degrees, up to 100 deg rate error, i_factor is almost 1 (having no effect),
    // and up to 200 deg error leads to <25% reduction of I.
    float i_factor = rate_error(i) / math::radians(400.f);
    i_factor = math::max(0.0f, 1.f - i_factor * i_factor);

    // Perform the integration using a first order method
    float rate_i = _rate_int(i) + i_factor * _gain_i(i) * rate_error(i) * dt;

    // do not propagate the result if out of range or invalid
    if (PX4_ISFINITE(rate_i)) {
      _rate_int(i) = math::constrain(rate_i, -_lim_int(i), _lim_int(i));
    }
  }
}
```

更新积分器，如果执行器饱和，则防止积分器深度饱和。

如果误差过大，抑制积分器，防止积分器进入深度饱和区。

### 发布力矩与推力`setpoint`

```CPP
vehicle_thrust_setpoint.timestamp_sample = angular_velocity.timestamp_sample;
vehicle_thrust_setpoint.timestamp = hrt_absolute_time();
_vehicle_thrust_setpoint_pub.publish(vehicle_thrust_setpoint);

vehicle_torque_setpoint.timestamp_sample = angular_velocity.timestamp_sample;
vehicle_torque_setpoint.timestamp = hrt_absolute_time();
_vehicle_torque_setpoint_pub.publish(vehicle_torque_setpoint);
```

推力`setpoint`直接来自`mc_att_control`发来的`vehicle_rates_setpoint`,该模块不做任何修改，因为角速度控制器是控制力矩的。
