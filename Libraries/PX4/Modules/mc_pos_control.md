# mc_pos_control

`mc_pos_control`模块是多旋翼无人机位置环，速度环`PID`控制器。它实现的功能如下

* 控制无人机起飞，与降落。
* 进行位置环的`P`控制，期望位置转化为期望速度。
* 进行速度环的`PID`控制，把期望速度转化为期望加速度。
* 同时加上用户设置的`setpoint`,也就是前馈控制。
* 把期望加速度按照多旋翼模型假设，转化为期望三轴推力。
* 把期望三轴推力，转化为期望四元数。

控制器的每一个环节都存在着限幅，防止无人机产生过度的控制信号。

## 关键数据流

### 订阅

#### `vehicle_local_position`

是这个模块的驱动订阅，表示当前无人机的三维位置，速度，加速度等信息，由`EKF`模块发布。

#### `trajectory_setpoint`

表示无人机的航迹`setpoint`,由`flight_mode_manager`或者是远程电脑发布。

#### `vehicle_control_mode`

决定是否使能当前控制器，由`commander`发布。

#### `vehicle_constraints`

决定是否起飞，以及飞行时上下的速度。由`flight_mode_manager`发布。

#### `hover_thrust_estimate`

使得无人机悬停的推力估计，推力是一个标准化的推力，范围为`[0,1]`,由`MulticopterHoverThrustEstimator`模块发布。

#### `vehicle_land_detected`

检测无人机是否落地，如果落地，需要处理控制器的输出，防止其再度起飞。由`land_detector`模块发布。

### 发布

#### `takeoff_status`

表示无人机现在的起飞状态与起飞阶段，是否起飞的信息。`flight_mode_manager`会使用。

#### `vehicle_attitude_setpoint`

表示无人机的期望姿态角，传递给姿态控制器。

#### `vehicle_local_position_setpoint`

表示该模块控制器最终使用的期望`setpoint`,来自`trajectory_setpoint`的`setpoint`可能会在模块内部被修改(比如，此时无人机还没有在飞行).`vehicle_local_position_setpoint`就表示了最终控制器使用的`setpoint`.`land_detector`可能使用这个来检测降落。

## MulticopterPositionControl类

```CPP
class MulticopterPositionControl : public ModuleBase<MulticopterPositionControl>, public ModuleParams,
  public px4::ScheduledWorkItem
```

`MulticopterPositionControl`类是在`nav_and_controllers`的工作队列中的模块。

### 关键成员变量

#### `_setpoint`

控制器实际使用的`setpoint`.这可能与订阅的不同，因为模块自身可能修改这个变量。

#### `_takeoff`

```CPP
TakeoffHandling _takeoff;
```

包含起飞控制的状态机，防止起飞时的无人机跳变。

#### `_control`

```CPP
PositionControl _control;
```

包含核心的`PID`控制环节。

### 关键参数

#### `COM_SPOOLUP_TIME`

在`commander_params.c`中定义，决定了在`arm`后必须要等待的时间，之后才能起飞。

#### `MPC_USE_HTE`

在`multicopter_position_control_params.c`中定义，决定是否使用悬停推力估计。

#### 控制器`PID`参数

```CPP
(ParamFloat<px4::params::MPC_XY_P>)         _param_mpc_xy_p,
 (ParamFloat<px4::params::MPC_Z_P>)          _param_mpc_z_p,
 (ParamFloat<px4::params::MPC_XY_VEL_P_ACC>) _param_mpc_xy_vel_p_acc,
 (ParamFloat<px4::params::MPC_XY_VEL_I_ACC>) _param_mpc_xy_vel_i_acc,
 (ParamFloat<px4::params::MPC_XY_VEL_D_ACC>) _param_mpc_xy_vel_d_acc,
 (ParamFloat<px4::params::MPC_Z_VEL_P_ACC>)  _param_mpc_z_vel_p_acc,
 (ParamFloat<px4::params::MPC_Z_VEL_I_ACC>)  _param_mpc_z_vel_i_acc,
 (ParamFloat<px4::params::MPC_Z_VEL_D_ACC>)  _param_mpc_z_vel_d_acc,
```

在`multicopter_position_control_gain_params.c`中定义，是控制器的`PID`参数。

## 代码分析

### 更新悬停推力

悬停推力指的是当无人机再某一个高度悬停时所需推力的值，这个值不是实际的值，而是一个标准化的值,范围为`[0,1]`.所谓悬停推力，实际上就是经过标准化的无人机重力，它是通过推力来估计出来的，所以叫做悬停推力。

```CPP
if (_param_mpc_use_hte.get()) {
  hover_thrust_estimate_s hte;

  if (_hover_thrust_estimate_sub.update(&hte)) {
    if (hte.valid) {
      _control.updateHoverThrust(hte.hover_thrust);
    }
  }
}
```

按照参数`MPC_USE_HTE`的设置，选择是否启用悬停推力估计。

```CPP
void PositionControl::updateHoverThrust(const float hover_thrust_new)
{
  const float previous_hover_thrust = _hover_thrust;
  setHoverThrust(hover_thrust_new);

  _vel_int(2) += (_acc_sp(2) - CONSTANTS_ONE_G) * previous_hover_thrust / _hover_thrust
           + CONSTANTS_ONE_G - _acc_sp(2);
}
```

更新了悬停推力后，为了缓慢地变化期望推力，修改积分器，减小改变悬停推理的影响。

根据牛顿运动定律，无人机的期望推力公式如下

$$
T = \frac{{}a_{sp}T_h}{g} - T_h
$$

其中，$a_{sp}$就是期望的加速度，$T_h$是悬停推力，方向是竖直向下。

### 更新状态

```CPP
PositionControlStates states{set_vehicle_states(vehicle_local_position, dt)};
```

从`vehicle_local_position`中获取当前无人机的位置，速度，同时检验有效性，滤波，计算出加速度。

```CPP
PositionControlStates MulticopterPositionControl::set_vehicle_states(const vehicle_local_position_s
    &vehicle_local_position, const float dt_s)
{
  PositionControlStates states;

  const Vector2f position_xy(vehicle_local_position.x, vehicle_local_position.y);

  // only set position states if valid and finite
  if (vehicle_local_position.xy_valid && position_xy.isAllFinite()) {
    states.position.xy() = position_xy;

  } else {
    states.position(0) = states.position(1) = NAN;
  }

  if (PX4_ISFINITE(vehicle_local_position.z) && vehicle_local_position.z_valid) {
    states.position(2) = vehicle_local_position.z;

  } else {
    states.position(2) = NAN;
  }

  const Vector2f velocity_xy(vehicle_local_position.vx, vehicle_local_position.vy);

  if (vehicle_local_position.v_xy_valid && velocity_xy.isAllFinite()) {
    const Vector2f vel_xy_prev = _vel_xy_lp_filter.getState();

    // vel xy notch filter, then low pass filter
    states.velocity.xy() = _vel_xy_lp_filter.update(_vel_xy_notch_filter.apply(velocity_xy)); //陷波滤波器+低通滤波器

    // vel xy derivative low pass filter
    states.acceleration.xy() = _vel_deriv_xy_lp_filter.update((_vel_xy_lp_filter.getState() - vel_xy_prev) / dt_s); // 低通滤波器

  } else {
    states.velocity(0) = states.velocity(1) = NAN;
    states.acceleration(0) = states.acceleration(1) = NAN;

    // reset filters to prevent acceleration spikes when regaining velocity
    _vel_xy_lp_filter.reset({});
    _vel_xy_notch_filter.reset();
    _vel_deriv_xy_lp_filter.reset({});
  }

  if (PX4_ISFINITE(vehicle_local_position.vz) && vehicle_local_position.v_z_valid) {

    const float vel_z_prev = _vel_z_lp_filter.getState();

    // vel z notch filter, then low pass filter
    states.velocity(2) = _vel_z_lp_filter.update(_vel_z_notch_filter.apply(vehicle_local_position.vz));

    // vel z derivative low pass filter
    states.acceleration(2) = _vel_deriv_z_lp_filter.update((_vel_z_lp_filter.getState() - vel_z_prev) / dt_s);

  } else {
    states.velocity(2) = NAN;
    states.acceleration(2) = NAN;

    // reset filters to prevent acceleration spikes when regaining velocity
    _vel_z_lp_filter.reset({});
    _vel_z_notch_filter.reset();
    _vel_deriv_z_lp_filter.reset({});
  }

  states.yaw = vehicle_local_position.heading;

  return states;
}
```

在赋值速度状态前，对速度进行了滤波，微分后获得加速度，防止微分放大噪声。

### 生成安全的`setpoint`

有时，需要获取安全的`setpoint`，防止无人机跑飞，目前的情况如下。

```CPP
if (_vehicle_control_mode.flag_multicopter_position_control_enabled) {
  // set failsafe setpoint if there hasn't been a new
  // trajectory setpoint since position control started
  if ((_setpoint.timestamp < _time_position_control_enabled)
    && (vehicle_local_position.timestamp_sample > _time_position_control_enabled)) {

    _setpoint = generateFailsafeSetpoint(vehicle_local_position.timestamp_sample, states, false);
    }
}
```

当无人机使能位置控制器时，已经有了一个`setpoint`了，这个`setpoint`不应该使用。

```CPP
// Run position control
if (!_control.update(dt)) {
  // Failsafe
  _vehicle_constraints = {0, NAN, NAN, false, {}}; // reset constraints

  _control.setInputSetpoint(generateFailsafeSetpoint(vehicle_local_position.timestamp_sample, states, true));
  _control.setVelocityLimits(_param_mpc_xy_vel_max.get(), _param_mpc_z_vel_max_up.get(), _param_mpc_z_vel_max_dn.get());
  _control.update(dt);
}
```

当位置控制器运行出错时，需要一个安全的`setpoint`来重新运行位置控制器。

```CPP
rajectory_setpoint_s MulticopterPositionControl::generateFailsafeSetpoint(const hrt_abstime &now,
    const PositionControlStates &states, bool warn)
{
  // rate limit the warnings
  warn = warn && (now - _last_warn) > 2_s;

  if (warn) {
    PX4_WARN("invalid setpoints");
    _last_warn = now;
  }

  trajectory_setpoint_s failsafe_setpoint = PositionControl::empty_trajectory_setpoint;
  failsafe_setpoint.timestamp = now;

  if (Vector2f(states.velocity).isAllFinite()) {
    // don't move along xy
    failsafe_setpoint.velocity[0] = failsafe_setpoint.velocity[1] = 0.f;

    if (warn) {
      PX4_WARN("Failsafe: stop and wait");
    }

  } else {
    // descend with land speed since we can't stop
    failsafe_setpoint.acceleration[0] = failsafe_setpoint.acceleration[1] = 0.f;
    failsafe_setpoint.velocity[2] = _param_mpc_land_speed.get();

    if (warn) {
      PX4_WARN("Failsafe: blind land");
    }
  }

  if (PX4_ISFINITE(states.velocity(2))) {
    // don't move along z if we can stop in all dimensions
    if (!PX4_ISFINITE(failsafe_setpoint.velocity[2])) {
      failsafe_setpoint.velocity[2] = 0.f;
    }

  } else {
    // emergency descend with a bit below hover thrust
    failsafe_setpoint.velocity[2] = NAN;
    failsafe_setpoint.acceleration[2] = .3f;

    if (warn) {
      PX4_WARN("Failsafe: blind descent");
    }
  }

  return failsafe_setpoint;
}
```

大致逻辑就是，如果当前状态有效，那么就悬停在当前位置。

### 管理起飞与降落

```CPP
TakeoffHandling _takeoff; /**< state machine and ramp to bring the vehicle off the ground without jumps */
```

这是一个状态机，在无人机起飞与降落之间的状态进行切换。

```CPP
void TakeoffHandling::updateTakeoffState(const bool armed, const bool landed, const bool want_takeoff,
    const float takeoff_desired_vz, const bool skip_takeoff, const hrt_abstime &now_us)
{
  _spoolup_time_hysteresis.set_state_and_update(armed, now_us);

  switch (_takeoff_state) {
  case TakeoffState::disarmed:
    if (armed) {
      _takeoff_state = TakeoffState::spoolup;

    } else {
      break;
    }

  // FALLTHROUGH
  case TakeoffState::spoolup:
    if (_spoolup_time_hysteresis.get_state()) {
      _takeoff_state = TakeoffState::ready_for_takeoff;

    } else {
      break;
    }

  // FALLTHROUGH
  case TakeoffState::ready_for_takeoff:
    if (want_takeoff) {
      _takeoff_state = TakeoffState::rampup;
      _takeoff_ramp_progress = 0.f;

    } else {
      break;
    }

  // FALLTHROUGH
  case TakeoffState::rampup:
    if (_takeoff_ramp_progress >= 1.f) {
      _takeoff_state = TakeoffState::flight;

    } else {
      break;
    }

  // FALLTHROUGH
  case TakeoffState::flight:
    if (landed) {
      _takeoff_state = TakeoffState::ready_for_takeoff;
    }

    break;

  default:
    break;
  }

  if (armed && skip_takeoff) {
    _takeoff_state = TakeoffState::flight;
  }

  // TODO: need to consider free fall here
  if (!armed) {
    _takeoff_state = TakeoffState::disarmed;
  }
}
```

这只是一个状态机，真正使得无人机起飞与降落的是这些订阅消息

* `_vehicle_control_mode.flag_armed`
* `_vehicle_land_detected.landed`
* `_vehicle_constraints.want_takeoff`,`_vehicle_constraints.speed_up`

```CPP
const bool not_taken_off             = (_takeoff.getTakeoffState() < TakeoffState::rampup);
const bool flying                    = (_takeoff.getTakeoffState() >= TakeoffState::flight);
const bool flying_but_ground_contact = (flying && _vehicle_land_detected.ground_contact);
```

这是通过`_takeoff`获取的无人机三个状态，在起飞与降落间还有一个飞行但是触地的状态。

```CPP
if (!flying) {
  _control.setHoverThrust(_param_mpc_thr_hover.get());
}

// make sure takeoff ramp is not amended by acceleration feed-forward
if (_takeoff.getTakeoffState() == TakeoffState::rampup && PX4_ISFINITE(_setpoint.velocity[2])) {
  _setpoint.acceleration[2] = NAN;
}

if (not_taken_off || flying_but_ground_contact) {
  // we are not flying yet and need to avoid any corrections
  _setpoint = PositionControl::empty_trajectory_setpoint;
   _setpoint.timestamp = vehicle_local_position.timestamp_sample;
  Vector3f(0.f, 0.f, 100.f).copyTo(_setpoint.acceleration); // High downwards acceleration to make sure there's no thrust

  // prevent any integrator windup
  _control.resetIntegral();
}
```

当无人机不起飞时，重置悬停推力。

当无人机上升时，为了防止急速上升，所以取消加速度的`setpoint`.

无人机不起飞或者是触地时，为了防止起飞发生，清空所有的`setpoint`,同时给加速度`setpoint`设置一个很大的值，防止起飞。同时重置积分器，防止积分饱和。

### 设置飞行限制

```CPP
// limit tilt during takeoff ramupup
const float tilt_limit_deg = (_takeoff.getTakeoffState() < TakeoffState::flight)
           ? _param_mpc_tiltmax_lnd.get() : _param_mpc_tiltmax_air.get();
_control.setTiltLimit(_tilt_limit_slew_rate.update(math::radians(tilt_limit_deg), dt));

const float speed_up = _takeoff.updateRamp(dt,
           PX4_ISFINITE(_vehicle_constraints.speed_up) ? _vehicle_constraints.speed_up : _param_mpc_z_vel_max_up.get());
const float speed_down = PX4_ISFINITE(_vehicle_constraints.speed_down) ? _vehicle_constraints.speed_down :
       _param_mpc_z_vel_max_dn.get();

// Allow ramping from zero thrust on takeoff
const float minimum_thrust = flying ? _param_mpc_thr_min.get() : 0.f;
_control.setThrustLimits(minimum_thrust, _param_mpc_thr_max.get());

float max_speed_xy = _param_mpc_xy_vel_max.get();

if (PX4_ISFINITE(vehicle_local_position.vxy_max)) {
    max_speed_xy = math::min(max_speed_xy, vehicle_local_position.vxy_max);
}


_control.setVelocityLimits(
  max_speed_xy,
  math::min(speed_up, _param_mpc_z_vel_max_up.get()), // takeoff ramp starts with negative velocity limit
  math::max(speed_down, 0.f));
```

设置飞行限制主要如下

* 设置倾斜角的最大值，这个取决于参数`MPC_TILTMAX_LND`,`MPC_TILTMAX_AIR`以及起飞状态。
* 设置上升与下降的最大速度，却决于参数与起飞状态。
* 设置最大的推力限制。
* 设置水平面最大飞行速度。

### 核心控制

```CPP
_control.update(dt);
```

```CPP
bool PositionControl::update(const float dt)
{
  bool valid = _inputValid();

  if (valid) {
    _positionControl();
    _velocityControl(dt);

    _yawspeed_sp = PX4_ISFINITE(_yawspeed_sp) ? _yawspeed_sp : 0.f;
    _yaw_sp = PX4_ISFINITE(_yaw_sp) ? _yaw_sp : _yaw; // TODO: better way to disable yaw control
  }

  // There has to be a valid output acceleration and thrust setpoint otherwise something went wrong
  return valid && _acc_sp.isAllFinite() && _thr_sp.isAllFinite();
}

```

`_inputValid`函数检测输入的正确性，注意，不是`setpoint`中有`NAN`就是不正确，而是对于一个xyz方向，至少要有一个`_pos_sp`,`_vel_sp`,`_acc_sp`合法，xy平面必须都合法，以及对应的状态应该是合法的。

直接设置偏航角期望，因为这不是`pos`控制器需要关心的内容。

#### 位置控制器

```CPP
void PositionControl::_positionControl()
{
  // P-position controller
  Vector3f vel_sp_position = (_pos_sp - _pos).emult(_gain_pos_p);
  // Position and feed-forward velocity setpoints or position states being NAN results in them not having an influence
  ControlMath::addIfNotNanVector3f(_vel_sp, vel_sp_position);
  // make sure there are no NAN elements for further reference while constraining
  ControlMath::setZeroIfNanVector3f(vel_sp_position);

  // Constrain horizontal velocity by prioritizing the velocity component along the
  // the desired position setpoint over the feed-forward term.
  _vel_sp.xy() = ControlMath::constrainXY(vel_sp_position.xy(), (_vel_sp - vel_sp_position).xy(), _lim_vel_horizontal);
  // Constrain velocity in z-direction.
  _vel_sp(2) = math::constrain(_vel_sp(2), -_lim_vel_up, _lim_vel_down);
}
```

位置控制公式如下

$$
\mathbb{V}_{sp} = (\mathbb{P}_{sp} - \mathbb{P}_{now})^TK_p
$$

位置控制是经典的`P`控制。

计算得出由位置`setpoint`产生的速度`setpoint`，加到设置的速度`setpoint`上去，如果是`NAN`则覆盖。

同时限制计算得出的速度`setpoint`的幅度。水平方向的限幅是XY轴同时考虑的。

#### 速度控制器

```CPP
void PositionControl::_velocityControl(const float dt)
{
  // Constrain vertical velocity integral
  _vel_int(2) = math::constrain(_vel_int(2), -CONSTANTS_ONE_G, CONSTANTS_ONE_G);

  // PID velocity control
  Vector3f vel_error = _vel_sp - _vel;
  Vector3f acc_sp_velocity = vel_error.emult(_gain_vel_p) + _vel_int - _vel_dot.emult(_gain_vel_d);

  // No control input from setpoints or corresponding states which are NAN
  ControlMath::addIfNotNanVector3f(_acc_sp, acc_sp_velocity);

  _accelerationControl();

  // Integrator anti-windup in vertical direction
  if ((_thr_sp(2) >= -_lim_thr_min && vel_error(2) >= 0.f) ||
      (_thr_sp(2) <= -_lim_thr_max && vel_error(2) <= 0.f)) {
    vel_error(2) = 0.f;
  }

  // Prioritize vertical control while keeping a horizontal margin
  const Vector2f thrust_sp_xy(_thr_sp);
  const float thrust_sp_xy_norm = thrust_sp_xy.norm();
  const float thrust_max_squared = math::sq(_lim_thr_max);

  // Determine how much vertical thrust is left keeping horizontal margin
  const float allocated_horizontal_thrust = math::min(thrust_sp_xy_norm, _lim_thr_xy_margin);
  const float thrust_z_max_squared = thrust_max_squared - math::sq(allocated_horizontal_thrust);

  // Saturate maximal vertical thrust
  _thr_sp(2) = math::max(_thr_sp(2), -sqrtf(thrust_z_max_squared));

  // Determine how much horizontal thrust is left after prioritizing vertical control
  const float thrust_max_xy_squared = thrust_max_squared - math::sq(_thr_sp(2));
  float thrust_max_xy = 0.f;

  if (thrust_max_xy_squared > 0.f) {
    thrust_max_xy = sqrtf(thrust_max_xy_squared);
  }

  // Saturate thrust in horizontal direction
  if (thrust_sp_xy_norm > thrust_max_xy) {
    _thr_sp.xy() = thrust_sp_xy / thrust_sp_xy_norm * thrust_max_xy;
  }

  // Use tracking Anti-Windup for horizontal direction: during saturation, the integrator is used to unsaturate the output
  // see Anti-Reset Windup for PID controllers, L.Rundqwist, 1990
  const Vector2f acc_sp_xy_produced = Vector2f(_thr_sp) * (CONSTANTS_ONE_G / _hover_thrust);

  // The produced acceleration can be greater or smaller than the desired acceleration due to the saturations and the actual vertical thrust (computed independently).
  // The ARW loop needs to run if the signal is saturated only.
  if (_acc_sp.xy().norm_squared() > acc_sp_xy_produced.norm_squared()) {
    const float arw_gain = 2.f / _gain_vel_p(0);
    const Vector2f acc_sp_xy = _acc_sp.xy();

    vel_error.xy() = Vector2f(vel_error) - arw_gain * (acc_sp_xy - acc_sp_xy_produced);
  }

  // Make sure integral doesn't get NAN
  ControlMath::setZeroIfNanVector3f(vel_error);
  // Update integral part of velocity control
  _vel_int += vel_error.emult(_gain_vel_i) * dt;
}
```

速度控制器进行经典的`PID`控制，并限制积分饱和

$$
\mathbb{A}_{sp} = \mathbb{V}_{err}\mathbb{K}_{p} + \mathbb{V}_{dot}\mathbb{K}_{d} + \mathbb{V}_{int}\mathbb{K}_{i}
$$

计算得出由位置`setpoint`产生的加速度`setpoint`，加到设置的加速度`setpoint`上去，如果是`NAN`则覆盖。

`_accelerationControl`函数按照旋翼无人机结构假设，把三轴加速度转换为三轴推力期望

### 将三轴推力转化为姿态角

```CPP
void PositionControl::getAttitudeSetpoint(vehicle_attitude_setpoint_s &attitude_setpoint) const
{
  ControlMath::thrustToAttitude(_thr_sp, _yaw_sp, attitude_setpoint);
  attitude_setpoint.yaw_sp_move_rate = _yawspeed_sp;
}
```

按照旋翼无人机假设，三轴推力的反方向就是期望机体z轴方向，再根据偏航角期望计算得出期望机体x轴方向，最后得出三个姿态角。
