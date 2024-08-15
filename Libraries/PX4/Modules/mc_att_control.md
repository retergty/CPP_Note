# mc_att_control

`mc_att_control`是多旋翼无人机的角度环的非线性控制器，它总是使能的，实现的功能如下

* 在`Manual`模式下，接收从`manual_control`发送来的`manual_control_setpoint`，并生成`attitude_setpoint`.
* 在位置控制器使能的情况下，接收从`mc_pos_control`发送来的`attitude_setpoint`.
* 将`attitude_setpoint`进行倾转分离，通过非线性`P`控制器获得`rate_setpoint`并发送给角速度控制器。

控制器的每一个环节都存在着限幅，防止无人机产生过度的控制信号。

## 关键数据流

### 订阅

#### `vehicle_attitude`

这是模块的驱动订阅，表示无人机当前的姿态信息，由`EKF`模块发布。

#### `manual_control_setpoint`

表示手柄操作时发布的`setpoint`,由`manual_control`发布，只在`Manual`模式下有效果。

#### `vehicle_control_mode`

表示控制器是否使能，对于旋翼无人机角度环与角加速度环总是使能的，由`commander`发布。模块使用这个订阅来决定是否为`Manual`模式，也就是`pos`控制器是否使能。如果为`Manual`模式，还需要生成`vehicle_attitude_setpoint`

#### `vehicle_attitude_setpoint`

表示无人机的姿态`setpoint`,由本模块或者是`mc_pos_control`模块发布。

#### `vehicle_status`

模块使用字段`vehicle_status.arming_state`来决定无人机是否`arm`.

#### `vehicle_local_position`

表示无人机当前的三维位置等诸多信息，由`EKF`模块发布，该模块使用`vehicle_local_position.heading_good_for_control`字段来决定手动模式下无人机的偏航角`setpoint`.

#### `autotune_attitude_control_status`

表示自动`PID`调节，由模块`mc_autotune_attitude_control`发布。

### 发布

#### `vehicle_rates_setpoint`

发布角速度`setpoint`，给`mc_rate_control`模块

#### `vehicle_attitude_setpoint`

在`Manual`模式下，发布,又由自身订阅。

## MulticopterAttitudeControl类

```CPP
class MulticopterAttitudeControl : public ModuleBase<MulticopterAttitudeControl>, public ModuleParams,
  public px4::WorkItem
```

`MulticopterAttitudeControl类`类是在`nav_and_controllers`的工作队列中的模块。

### 关键成员变量

#### `_attitude_control`

```CPP
AttitudeControl _attitude_control;
```

核心的姿态控制。

#### `_man_roll_input_filter`,`_man_pitch_input_filter`

```CPP
AlphaFilter<float> _man_roll_input_filter;
AlphaFilter<float> _man_pitch_input_filter;
```

手动操作时，控制滚转与俯仰角的输入的低通滤波器。

### 关键参数

#### `COM_SPOOLUP_TIME`

在`commander_params.c`中定义，在手动模式下使用，决定了在`arm`后必须要等待的时间，之后才能起飞。

#### 姿态`P`值

```CPP
(ParamFloat<px4::params::MC_ROLL_P>)        _param_mc_roll_p,
(ParamFloat<px4::params::MC_PITCH_P>)       _param_mc_pitch_p,
(ParamFloat<px4::params::MC_YAW_P>)         _param_mc_yaw_p,
```

在`mc_att_control_params.c`中定义，就是角度控制器的`P`值。

#### `MC_YAW_WEIGHT`

在`mc_att_control_params.c`中定义，倾转分离后，抑制偏航角的幅度。

## 代码分析

### 管理`Manual`模式下的遥控器输入

```CPP
// Generate the attitude setpoint from stick inputs if we are in Manual/Stabilized mode
if (_vehicle_control_mode.flag_control_manual_enabled &&
    !_vehicle_control_mode.flag_control_altitude_enabled &&
    !_vehicle_control_mode.flag_control_velocity_enabled &&
    !_vehicle_control_mode.flag_control_position_enabled) {

  generate_attitude_setpoint(q, dt, _reset_yaw_sp);
  attitude_setpoint_generated = true;

} else {
  _man_roll_input_filter.reset(0.f);
  _man_pitch_input_filter.reset(0.f);
}
```

在手动模式下，接受`manual_control`模块发送的`manual_control_setpoint`，生成`vehicle_attitude_setpoint`。

```CPP
void
MulticopterAttitudeControl::generate_attitude_setpoint(const Quatf &q, float dt, bool reset_yaw_sp)
{
  vehicle_attitude_setpoint_s attitude_setpoint{};
  const float yaw = Eulerf(q).psi();

  attitude_setpoint.yaw_sp_move_rate = _manual_control_setpoint.yaw * math::radians(_param_mpc_man_y_max.get());

  // Avoid accumulating absolute yaw error with arming stick gesture in case heading_good_for_control stays true
  if ((_manual_control_setpoint.throttle < -.9f) && (_param_mc_airmode.get() != 2)) {
    reset_yaw_sp = true;
  }

  // Make sure not absolute heading error builds up
  if (reset_yaw_sp) {
    _man_yaw_sp = yaw;

  } else {
    _man_yaw_sp = wrap_pi(_man_yaw_sp + attitude_setpoint.yaw_sp_move_rate * dt);
  }

  /*
   * Input mapping for roll & pitch setpoints
   * ----------------------------------------
   * We control the following 2 angles:
   * - tilt angle, given by sqrt(roll*roll + pitch*pitch)
   * - the direction of the maximum tilt in the XY-plane, which also defines the direction of the motion
   *
   * This allows a simple limitation of the tilt angle, the vehicle flies towards the direction that the stick
   * points to, and changes of the stick input are linear.
   */
  _man_roll_input_filter.setParameters(dt, _param_mc_man_tilt_tau.get());
  _man_pitch_input_filter.setParameters(dt, _param_mc_man_tilt_tau.get());

  // we want to fly towards the direction of (roll, pitch)
  Vector2f v = Vector2f(_man_roll_input_filter.update(_manual_control_setpoint.roll * _man_tilt_max),
            -_man_pitch_input_filter.update(_manual_control_setpoint.pitch * _man_tilt_max));
  float v_norm = v.norm(); // the norm of v defines the tilt angle

  if (v_norm > _man_tilt_max) { // limit to the configured maximum tilt angle
    v *= _man_tilt_max / v_norm;
  }

  Quatf q_sp_rp = AxisAnglef(v(0), v(1), 0.f);
  // The axis angle can change the yaw as well (noticeable at higher tilt angles).
  // This is the formula by how much the yaw changes:
  //   let a := tilt angle, b := atan(y/x) (direction of maximum tilt)
  //   yaw = atan(-2 * sin(b) * cos(b) * sin^2(a/2) / (1 - 2 * cos^2(b) * sin^2(a/2))).
  const Quatf q_sp_yaw(cosf(_man_yaw_sp / 2.f), 0.f, 0.f, sinf(_man_yaw_sp / 2.f));

  if (_vtol) {
    // Modify the setpoints for roll and pitch such that they reflect the user's intention even
    // if a large yaw error(yaw_sp - yaw) is present. In the presence of a yaw error constructing
    // an attitude setpoint from the yaw setpoint will lead to unexpected attitude behaviour from
    // the user's view as the tilt will not be aligned with the heading of the vehicle.

    AttitudeControlMath::correctTiltSetpointForYawError(q_sp_rp, q, q_sp_yaw);
  }

  // Align the desired tilt with the yaw setpoint
  Quatf q_sp = q_sp_yaw * q_sp_rp;

  q_sp.copyTo(attitude_setpoint.q_d);

  // Transform to euler angles for logging only
  const Eulerf euler_sp(q_sp);
  attitude_setpoint.roll_body = euler_sp(0);
  attitude_setpoint.pitch_body = euler_sp(1);
  attitude_setpoint.yaw_body = euler_sp(2);

  attitude_setpoint.thrust_body[2] = -throttle_curve(_manual_control_setpoint.throttle);

  attitude_setpoint.timestamp = hrt_absolute_time();
  _vehicle_attitude_setpoint_pub.publish(attitude_setpoint);
}
```

在滚转与俯仰通道上进行低通滤波，同时保证不会超过最大倾斜角。

### 进行姿态角控制

```CPP
Vector3f rates_sp = _attitude_control.update(q);
```

进行非线性姿态角控制。

```CPP
matrix::Vector3f AttitudeControl::update(const Quatf &q) const
{
  Quatf qd = _attitude_setpoint_q;

  // calculate reduced desired attitude neglecting vehicle's yaw to prioritize roll and pitch
  const Vector3f e_z = q.dcm_z();
  const Vector3f e_z_d = qd.dcm_z();
  Quatf qd_red(e_z, e_z_d);

  if (fabsf(qd_red(1)) > (1.f - 1e-5f) || fabsf(qd_red(2)) > (1.f - 1e-5f)) {
    // In the infinitesimal corner case where the vehicle and thrust have the completely opposite direction,
    // full attitude control anyways generates no yaw input and directly takes the combination of
    // roll and pitch leading to the correct desired yaw. Ignoring this case would still be totally safe and stable.
    qd_red = qd;

  } else {
    // transform rotation from current to desired thrust vector into a world frame reduced desired attitude
    qd_red *= q;
  }

  // mix full and reduced desired attitude
  Quatf q_mix = qd_red.inversed() * qd;
  q_mix.canonicalize();
  // catch numerical problems with the domain of acosf and asinf
  q_mix(0) = math::constrain(q_mix(0), -1.f, 1.f);
  q_mix(3) = math::constrain(q_mix(3), -1.f, 1.f);
  qd = qd_red * Quatf(cosf(_yaw_w * acosf(q_mix(0))), 0, 0, sinf(_yaw_w * asinf(q_mix(3))));

  // quaternion attitude control law, qe is rotation from q to qd
  const Quatf qe = q.inversed() * qd;

  // using sin(alpha/2) scaled rotation axis as attitude error (see quaternion definition by axis angle)
  // also taking care of the antipodal unit quaternion ambiguity
  const Vector3f eq = 2.f * qe.canonical().imag();

  // calculate angular rates setpoint
  Vector3f rate_setpoint = eq.emult(_proportional_gain);

  // Feed forward the yaw setpoint rate.
  // yawspeed_setpoint is the feed forward commanded rotation around the world z-axis,
  // but we need to apply it in the body frame (because _rates_sp is expressed in the body frame).
  // Therefore we infer the world z-axis (expressed in the body frame) by taking the last column of R.transposed (== q.inversed)
  // and multiply it by the yaw setpoint rate (yawspeed_setpoint).
  // This yields a vector representing the commanded rotatation around the world z-axis expressed in the body frame
  // such that it can be added to the rates setpoint.
  if (std::isfinite(_yawspeed_setpoint)) {
    rate_setpoint += q.inversed().dcm_z() * _yawspeed_setpoint;
  }

  // limit rates
  for (int i = 0; i < 3; i++) {
    rate_setpoint(i) = math::constrain(rate_setpoint(i), -_rate_limit(i), _rate_limit(i));
  }

  return rate_setpoint;
}
```

大致逻辑就是进行倾转分离，抑制偏航，得到误差轴角，误差轴角进一步角加速度`setpoint`.

这一个控制器是非线性的原因就是，每当无人机旋转了一个角度后，转动轴就变为了新的机体坐标系下的表示，与之前的就不同了。

得到角加速度`setpoint`后，还有前馈的偏航角速度`setpoint`.最后通过一个限幅，得到最终的角加速度`setpoint`.

### 接收PID自动整定的消息

```CPP
if (_autotune_attitude_control_status_sub.copy(&pid_autotune)) {
  if ((pid_autotune.state == autotune_attitude_control_status_s::STATE_ROLL
       || pid_autotune.state == autotune_attitude_control_status_s::STATE_PITCH
       || pid_autotune.state == autotune_attitude_control_status_s::STATE_YAW
       || pid_autotune.state == autotune_attitude_control_status_s::STATE_TEST)
      && ((now - pid_autotune.timestamp) < 1_s)) {
    rates_sp += Vector3f(pid_autotune.rate_sp);
  }
}
```

### 发布`vehicle_rates_setpoint`

```CPP
// publish rate setpoint
vehicle_rates_setpoint_s rates_setpoint{};
rates_setpoint.roll = rates_sp(0);
rates_setpoint.pitch = rates_sp(1);
rates_setpoint.yaw = rates_sp(2);
_thrust_setpoint_body.copyTo(rates_setpoint.thrust_body);
rates_setpoint.timestamp = hrt_absolute_time();

_vehicle_rates_setpoint_pub.publish(rates_setpoint);
```

发布`vehicle_rates_setpoint`,其中`roll`,`pitch`,`yaw`表示三个欧拉角设定的角速度，`thrust_body`则来自`mc_pos_control`或者是手动输入的，这个模块不会修改这个推力值，因为这个只会影响无人机上升或者下降，姿态变化是电机转速差决定的。
