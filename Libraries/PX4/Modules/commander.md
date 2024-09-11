# commander

`commander`是飞行任务栈命令执行的抽象.它接收主机的命令并执行，向主机发送命令执行结果.在主机端，通过终端`commander [command] [args]`便可以执行`commander`里特定的命令，比如传感器矫正，自动起飞等.

它常见执行的命令如下

* 接收主机`commander calibrate <sensor>`类型的命令，进行传感器校正.
* 接收主机`commander arm|disarm`命令，`arm`或者`disarm`飞行器.
* 接收主机`commander takeoff|land`命令，进行无人机自动起飞或者自动降落.
* 接收主机`commander mode <mode>`类型的命令，切换无人机飞行模式.

在处理主机命令之外，它还负责的功能如下

* 监视飞控系统，进行健康检查，在适当时候(比如检测出错误时)进行相应的动作，比如切换飞行模式等.这是由关键成员`_health_and_arming_checks`进行具体检查的.
* 管理`vehicle_status_s`与`vehicle_control_mode_s`两个关键的飞控消息,按照主机命令，当前状态，飞机类型发布正确的消息.
* 进行手动飞行检查，在自动模式时可以自动切换到手动增稳模式.
* 进行`offboard`飞行检查，在发生错误时及时做出反应
* 维护与主机间的数据链接，并在高延迟时及时做出反应
* 发布`failure_detector_status_s`，表示当前检测到的错误.

## 关键数据流

### 订阅

#### `vehicle_command`

表示当前模块需要执行的命令.

#### `manual_control_setpoint`

用在手动飞行检查中，在自动模式时，推动摇杆可以自动进入位置增稳的辅助飞行模式.

#### `offboard_control_mode`

用在`offboard`飞行检查，在发生错误时及时做出反应

### 发布

#### `vehicle_status`

重要的飞控消息，表示无人机飞行的种种状态，发布给其它模块.

#### `vehicle_control_mode`

重要的飞控消息，表示无人机控制器使能情况,由`vehicle_status`与飞机类型直接决定,发布给其它模块进行飞行控制.

#### `failure_detector_status`

表示检测到的具体错误，发布给其它具体的模块进行错误处理.

## Commander类

```CPP
class Commander : public ModuleBase<Commander>, public ModuleParams
```

`Commander`是独占一个线程的类.

### 关键成员变量

#### `_health_and_arming_checks`

```CPP
HealthAndArmingChecks  _health_and_arming_checks{this, _vehicle_status};
```

负责具体的各个子系统的检查.

#### `_failsafe_flags`

```CPP
const failsafe_flags_s &_failsafe_flags{_health_and_arming_checks.failsafeFlags()};
```

直接引用了`_health_and_arming_checks`的`_failsafe_flags`，方便后续报告错误.

### 关键成员函数

#### `run`

```CPP
void run() override;
```

这个函数进行复杂的管理，实现上述提到的功能》

#### `custom_command`

```CPP
static int custom_command(int argc, char *argv[]);
```

这个函数就是把主机传递来的命令转化为`uORB`主题消息`vehicle_command`发布，并由当前的类接收.

## 代码分析

### 处理主机传递的命令

```CPP
int Commander::custom_command(int argc, char *argv[])
{
  if (!is_running()) {
    print_usage("not running");
    return 1;
  }

#ifndef CONSTRAINED_FLASH

  if (!strcmp(argv[0], "calibrate")) {
    if (argc > 1) {
      if (!strcmp(argv[1], "gyro")) {
        // gyro calibration: param1 = 1
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 1.f, 0.f, 0.f, 0.f, 0.0, 0.0, 0.f);

      } else if (!strcmp(argv[1], "mag")) {
        if (argc > 2 && (strcmp(argv[2], "quick") == 0)) {
          // magnetometer quick calibration: VEHICLE_CMD_FIXED_MAG_CAL_YAW
          send_vehicle_command(vehicle_command_s::VEHICLE_CMD_FIXED_MAG_CAL_YAW);

        } else {
          // magnetometer calibration: param2 = 1
          send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 1.f, 0.f, 0.f, 0.0, 0.0, 0.f);
        }

      } else if (!strcmp(argv[1], "baro")) {
        // baro calibration: param3 = 1
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 1.f, 0.f, 0.0, 0.0, 0.f);

      } else if (!strcmp(argv[1], "accel")) {
        if (argc > 2 && (strcmp(argv[2], "quick") == 0)) {
          // accelerometer quick calibration: param5 = 3
          send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 0.f, 0.f, 4.0, 0.0, 0.f);

        } else {
          // accelerometer calibration: param5 = 1
          send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 0.f, 0.f, 1.0, 0.0, 0.f);
        }

      } else if (!strcmp(argv[1], "level")) {
        // board level calibration: param5 = 2
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 0.f, 0.f, 2.0, 0.0, 0.f);

      } else if (!strcmp(argv[1], "airspeed")) {
        // airspeed calibration: param6 = 2
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 0.f, 0.f, 0.0, 2.0, 0.f);

      } else if (!strcmp(argv[1], "esc")) {
        // ESC calibration: param7 = 1
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_CALIBRATION, 0.f, 0.f, 0.f, 0.f, 0.0, 0.0, 1.f);

      } else {
        PX4_ERR("argument %s unsupported.", argv[1]);
        return 1;
      }

      return 0;

    } else {
      PX4_ERR("missing argument");
    }
  }

  if (!strcmp(argv[0], "check")) {
    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_RUN_PREARM_CHECKS);

    uORB::SubscriptionData<vehicle_status_s> vehicle_status_sub{ORB_ID(vehicle_status)};
    PX4_INFO("Preflight check: %s", vehicle_status_sub.get().pre_flight_checks_pass ? "OK" : "FAILED");

    return 0;
  }

  if (!strcmp(argv[0], "arm")) {
    float param2 = 0.f;

    // 21196: force arming/disarming (e.g. allow arming to override preflight checks and disarming in flight)
    if (argc > 1 && !strcmp(argv[1], "-f")) {
      param2 = 21196.f;
    }

    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_COMPONENT_ARM_DISARM,
             static_cast<float>(vehicle_command_s::ARMING_ACTION_ARM),
             param2);

    return 0;
  }

  if (!strcmp(argv[0], "disarm")) {
    float param2 = 0.f;

    // 21196: force arming/disarming (e.g. allow arming to override preflight checks and disarming in flight)
    if (argc > 1 && !strcmp(argv[1], "-f")) {
      param2 = 21196.f;
    }

    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_COMPONENT_ARM_DISARM,
             static_cast<float>(vehicle_command_s::ARMING_ACTION_DISARM),
             param2);

    return 0;
  }

  if (!strcmp(argv[0], "takeoff")) {
    // switch to takeoff mode and arm
    uORB::SubscriptionData<vehicle_command_ack_s> vehicle_command_ack_sub{ORB_ID(vehicle_command_ack)};
    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_NAV_TAKEOFF);

    if (wait_for_vehicle_command_reply(vehicle_command_s::VEHICLE_CMD_NAV_TAKEOFF, vehicle_command_ack_sub)) {
      send_vehicle_command(vehicle_command_s::VEHICLE_CMD_COMPONENT_ARM_DISARM,
               static_cast<float>(vehicle_command_s::ARMING_ACTION_ARM),
               0.f);
    }

    return 0;
  }

  if (!strcmp(argv[0], "land")) {
    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_NAV_LAND);

    return 0;
  }

  if (!strcmp(argv[0], "transition")) {
    uORB::Subscription vehicle_status_sub{ORB_ID(vehicle_status)};
    vehicle_status_s vehicle_status{};
    vehicle_status_sub.copy(&vehicle_status);
    send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_VTOL_TRANSITION,
             (float)(vehicle_status.vehicle_type == vehicle_status_s::VEHICLE_TYPE_ROTARY_WING ?
               vtol_vehicle_status_s::VEHICLE_VTOL_STATE_FW :
               vtol_vehicle_status_s::VEHICLE_VTOL_STATE_MC), 0.0f);

    return 0;
  }

  if (!strcmp(argv[0], "mode")) {
    if (argc > 1) {

      if (!strcmp(argv[1], "manual")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_MANUAL);

      } else if (!strcmp(argv[1], "altctl")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_ALTCTL);

      } else if (!strcmp(argv[1], "posctl")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_POSCTL);

      } else if (!strcmp(argv[1], "position:slow")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_POSCTL,
                 PX4_CUSTOM_SUB_MODE_POSCTL_SLOW);

      } else if (!strcmp(argv[1], "auto:mission")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_MISSION);

      } else if (!strcmp(argv[1], "auto:loiter")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_LOITER);

      } else if (!strcmp(argv[1], "auto:rtl")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_RTL);

      } else if (!strcmp(argv[1], "acro")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_ACRO);

      } else if (!strcmp(argv[1], "offboard")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_OFFBOARD);

      } else if (!strcmp(argv[1], "stabilized")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_STABILIZED);

      } else if (!strcmp(argv[1], "auto:takeoff")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF);

      } else if (!strcmp(argv[1], "auto:land")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_LAND);

      } else if (!strcmp(argv[1], "auto:precland")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_AUTO_PRECLAND);

      } else if (!strcmp(argv[1], "ext1")) {
        send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_SET_MODE, 1, PX4_CUSTOM_MAIN_MODE_AUTO,
                 PX4_CUSTOM_SUB_MODE_EXTERNAL1);

      } else {
        PX4_ERR("argument %s unsupported.", argv[1]);
      }

      return 0;

    } else {
      PX4_ERR("missing argument");
    }
  }

  if (!strcmp(argv[0], "lockdown")) {

    if (argc < 2) {
      Commander::print_usage("not enough arguments, missing [on, off]");
      return 1;
    }

    bool ret = send_vehicle_command(vehicle_command_s::VEHICLE_CMD_DO_FLIGHTTERMINATION,
            strcmp(argv[1], "off") ? 2.0f : 0.0f /* lockdown */, 0.0f);

    return (ret ? 0 : 1);
  }

  if (!strcmp(argv[0], "pair")) {

    // GCS pairing request handled by a companion
    bool ret = broadcast_vehicle_command(vehicle_command_s::VEHICLE_CMD_START_RX_PAIR, 10.f);

    return (ret ? 0 : 1);
  }

  if (!strcmp(argv[0], "set_ekf_origin")) {
    if (argc > 3) {

      double latitude  = atof(argv[1]);
      double longitude = atof(argv[2]);
      float  altitude  = atof(argv[3]);

      // Set the ekf NED origin global coordinates.
      bool ret = send_vehicle_command(vehicle_command_s::VEHICLE_CMD_SET_GPS_GLOBAL_ORIGIN,
              0.f, 0.f, 0.0, 0.0, latitude, longitude, altitude);
      return (ret ? 0 : 1);

    } else {
      PX4_ERR("missing argument");
      return 0;
    }
  }

  if (!strcmp(argv[0], "poweroff")) {

    bool ret = send_vehicle_command(vehicle_command_s::VEHICLE_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
            2.0f);

    return (ret ? 0 : 1);
  }


#endif

  return print_usage("unknown command");
}
```

`custom_command`函数把主机传递来的命令转化为`uORB`主题消息`vehicle_command`发布，并由当前的类接收.

在主机终端上形如`commander ...`的命令实际上便是调用了`Commander::custom_command`函数，把这些命令读取并处理为`uORB`消息.

### 进行健康检查

```CPP
bool HealthAndArmingChecks::update(bool force_reporting)
{
  _reporter.reset();

  _reporter.prepare(_context.status().vehicle_type);

  for (unsigned i = 0; i < sizeof(_checks) / sizeof(_checks[0]); ++i) {
    if (!_checks[i]) {
      break;
    }

    _checks[i]->checkAndReport(_context, _reporter);
  }

  const bool results_changed = _reporter.finalize();
  const bool reported = _reporter.report(_context.isArmed(), force_reporting);

  if (reported) {

    // LEGACY start
    // Run the checks again, this time with the mavlink publication set.
    // We don't expect any change, and rate limitation would prevent the events from being reported again,
    // so we only report mavlink_log_*.
    _reporter._mavlink_log_pub = &_mavlink_log_pub;
    _reporter.reset();

    _reporter.prepare(_context.status().vehicle_type);

    for (unsigned i = 0; i < sizeof(_checks) / sizeof(_checks[0]); ++i) {
      if (!_checks[i]) {
        break;
      }

      _checks[i]->checkAndReport(_context, _reporter);
    }

    _reporter.finalize();
    _reporter.report(_context.isArmed(), false);
    _reporter._mavlink_log_pub = nullptr;
    // LEGACY end

    health_report_s health_report;
    _reporter.getHealthReport(health_report);
    health_report.timestamp = hrt_absolute_time();
    _health_report_pub.publish(health_report);
  }

  // Check if we need to publish the failsafe flags
  const hrt_abstime now = hrt_absolute_time();

  if ((now > _failsafe_flags.timestamp + 500_ms) || results_changed) {
    _failsafe_flags.timestamp = hrt_absolute_time();
    _failsafe_flags_pub.publish(_failsafe_flags);
  }

  return reported;
}
```

类成员`_health_and_arming_checks`的成员函数`update`检查并报告子系统的健康情况.具体检查的子系统由其类型`HealthAndArmingChecks`成员定义

```CPP
// all checks
AccelerometerChecks _accelerometer_checks;
AirspeedChecks _airspeed_checks;
ArmPermissionChecks _arm_permission_checks;
BaroChecks _baro_checks;
CpuResourceChecks _cpu_resource_checks;
DistanceSensorChecks _distance_sensor_checks;
EscChecks _esc_checks;
EstimatorChecks _estimator_checks;
FailureDetectorChecks _failure_detector_checks;
GyroChecks _gyro_checks;
ImuConsistencyChecks _imu_consistency_checks;
LoggerChecks _logger_checks;
MagnetometerChecks _magnetometer_checks;
ManualControlChecks _manual_control_checks;
HomePositionChecks _home_position_checks;
ModeChecks _mode_checks;
OpenDroneIDChecks _open_drone_id_checks;
ParachuteChecks _parachute_checks;
PowerChecks _power_checks;
RcCalibrationChecks _rc_calibration_checks;
SdCardChecks _sd_card_checks;
SystemChecks _system_checks;
BatteryChecks _battery_checks;
WindChecks _wind_checks;
GeofenceChecks _geofence_checks;
FlightTimeChecks _flight_time_checks;
MissionChecks _mission_checks;
RcAndDataLinkChecks _rc_and_data_link_checks;
VtolChecks _vtol_checks;
OffboardChecks _offboard_checks;
ExternalChecks _external_checks;
```

### 进行手动飞行切换

```CPP
void Commander::manualControlCheck()
{
  manual_control_setpoint_s manual_control_setpoint;
  const bool manual_control_updated = _manual_control_setpoint_sub.update(&manual_control_setpoint);

  if (manual_control_updated && manual_control_setpoint.valid) {

    _is_throttle_above_center = (manual_control_setpoint.throttle > 0.2f);
    _is_throttle_low = (manual_control_setpoint.throttle < -0.8f);

    if (isArmed()) {
      // Abort autonomous mode and switch to position mode if sticks are moved significantly
      // but only if actually in air.
      if (manual_control_setpoint.sticks_moving
          && !_vehicle_control_mode.flag_control_manual_enabled
          && (_vehicle_status.vehicle_type == vehicle_status_s::VEHICLE_TYPE_ROTARY_WING)
         ) {
        bool override_enabled = false;

        if (_vehicle_control_mode.flag_control_auto_enabled) {
          if (_param_com_rc_override.get() & static_cast<int32_t>(RcOverrideBits::AUTO_MODE_BIT)) {
            override_enabled = true;
          }
        }

        if (_vehicle_control_mode.flag_control_offboard_enabled) {
          if (_param_com_rc_override.get() & static_cast<int32_t>(RcOverrideBits::OFFBOARD_MODE_BIT)) {
            override_enabled = true;
          }
        }

        if (override_enabled) {
          // If no failsafe is active, directly change the mode, otherwise pass the request to the failsafe state machine
          if (_failsafe.selectedAction() <= FailsafeBase::Action::Warn) {
            if (_user_mode_intention.change(vehicle_status_s::NAVIGATION_STATE_POSCTL, ModeChangeSource::User, true)) {
              tune_positive(true);
              mavlink_log_info(&_mavlink_log_pub, "Pilot took over using sticks\t");
              events::send(events::ID("commander_rc_override"), events::Log::Info, "Pilot took over using sticks");
            }

          } else {
            _failsafe_user_override_request = true;
          }
        }
      }

    } else {
      const bool is_mavlink = (manual_control_setpoint.data_source > manual_control_setpoint_s::SOURCE_RC);

      // if there's never been a mode change force position control as initial state
      if (!_user_mode_intention.everHadModeChange() && (is_mavlink || !_mode_switch_mapped)) {
        _user_mode_intention.change(vehicle_status_s::NAVIGATION_STATE_POSCTL, ModeChangeSource::User, false, true);
      }
    }
  }
}
```

检查在自动模式下，是否用户尝试推动手柄接管无人机，如果是，切换状态为`NAVIGATION_STATE_POSCTL`.

### 进行`offboard`飞行检查

```CPP
void Commander::offboardControlCheck()
{
  if (_offboard_control_mode_sub.update()) {
    if (_failsafe_flags.offboard_control_signal_lost) {
      // Run arming checks immediately to allow for offboard mode activation
      _status_changed = true;
    }
  }
}
```

如果发生了`offboard`飞行时信号丢失的问题，报告这个消息.

### 处理命令

函数

```CPP
bool Commander::handle_command(const vehicle_command_s &cmd)
```

具体执行处理命令.

#### 切换飞行模式

在主机发送`commander mode <mode>`类型的命令后

```CPP
case vehicle_command_s::VEHICLE_CMD_DO_SET_MODE: {
  ....
}
```

`case vehicle_command_s::VEHICLE_CMD_DO_SET_MODE`执行设置`vehicle_status`中的`nav_state`的字段.

#### 自动起飞

```CPP
case vehicle_command_s::VEHICLE_CMD_NAV_TAKEOFF: {
  /* ok, home set, use it to take off */
  if (_user_mode_intention.change(vehicle_status_s::NAVIGATION_STATE_AUTO_TAKEOFF, getSourceFromCommand(cmd))) {
    cmd_result = vehicle_command_ack_s::VEHICLE_CMD_RESULT_ACCEPTED;

  } else {
    printRejectMode(vehicle_status_s::NAVIGATION_STATE_AUTO_TAKEOFF);
    cmd_result = vehicle_command_ack_s::VEHICLE_CMD_RESULT_TEMPORARILY_REJECTED;
  }
  }
  break;
```

`case vehicle_command_s::VEHICLE_CMD_NAV_TAKEOFF`管理自动起飞，实际上是通过把`vehicle_status`中的`nav_state`字段修改为`NAVIGATION_STATE_AUTO_TAKEOFF`实现的，之后由`flight_mode_manager`模块中的`AUTO`飞行模式管理自动起飞.

#### 自动降落

```CPP
case vehicle_command_s::VEHICLE_CMD_NAV_LAND: {
    if (_user_mode_intention.change(vehicle_status_s::NAVIGATION_STATE_AUTO_LAND, getSourceFromCommand(cmd))) {
      mavlink_log_info(&_mavlink_log_pub, "Landing at current position\t");
      events::send(events::ID("commander_landing_current_pos"), events::Log::Info,
              "Landing at current position");
      cmd_result = vehicle_command_ack_s::VEHICLE_CMD_RESULT_ACCEPTED;

    } else {
      printRejectMode(vehicle_status_s::NAVIGATION_STATE_AUTO_LAND);
      cmd_result = vehicle_command_ack_s::VEHICLE_CMD_RESULT_TEMPORARILY_REJECTED;
    }
  }
  break;
```

`case vehicle_command_s::VEHICLE_CMD_NAV_TAKEOFF`管理自动起飞，实际上是通过把`vehicle_status`中的`nav_state`字段修改为`NAVIGATION_STATE_AUTO_LAND`实现的，之后由`flight_mode_manager`模块中的`AUTO`飞行模式管理自动起飞.

### 进行延迟的`vehicle_status`修改

处理命令时不会立即对`vehicle_status`进行修改，而是等待函数

```CPP
bool Commander::handleModeIntentionAndFailsafe()
```

运行时再一次性检查是否可以修改.

### 更新`vehicle_control_mode`并发布

```CPP
void Commander::updateControlMode()
{
  _vehicle_control_mode = {};

  mode_util::getVehicleControlMode(_vehicle_status.nav_state,
           _vehicle_status.vehicle_type, _offboard_control_mode_sub.get(), _vehicle_control_mode);
  _mode_management.updateControlMode(_vehicle_status.nav_state, _vehicle_control_mode);

  _vehicle_control_mode.flag_armed = isArmed();
  _vehicle_control_mode.flag_multicopter_position_control_enabled =
    (_vehicle_status.vehicle_type == vehicle_status_s::VEHICLE_TYPE_ROTARY_WING)
    && (_vehicle_control_mode.flag_control_altitude_enabled
        || _vehicle_control_mode.flag_control_climb_rate_enabled
        || _vehicle_control_mode.flag_control_position_enabled
        || _vehicle_control_mode.flag_control_velocity_enabled
        || _vehicle_control_mode.flag_control_acceleration_enabled);
  _vehicle_control_mode.timestamp = hrt_absolute_time();
  _vehicle_control_mode_pub.publish(_vehicle_control_mode);
}
```

```CPP
void getVehicleControlMode(uint8_t nav_state, uint8_t vehicle_type,
         const offboard_control_mode_s &offboard_control_mode,
         vehicle_control_mode_s &vehicle_control_mode)
{

  switch (nav_state) {
  case vehicle_status_s::NAVIGATION_STATE_MANUAL:
    vehicle_control_mode.flag_control_manual_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = stabilization_required(vehicle_type);
    vehicle_control_mode.flag_control_rates_enabled = stabilization_required(vehicle_type);
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_STAB:
    vehicle_control_mode.flag_control_manual_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_ALTCTL:
    vehicle_control_mode.flag_control_manual_enabled = true;
    vehicle_control_mode.flag_control_altitude_enabled = true;
    vehicle_control_mode.flag_control_climb_rate_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_POSCTL:
  case vehicle_status_s::NAVIGATION_STATE_POSITION_SLOW:
    vehicle_control_mode.flag_control_manual_enabled = true;
    vehicle_control_mode.flag_control_position_enabled = true;
    vehicle_control_mode.flag_control_velocity_enabled = true;
    vehicle_control_mode.flag_control_altitude_enabled = true;
    vehicle_control_mode.flag_control_climb_rate_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_AUTO_RTL:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_LAND:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_PRECLAND:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_MISSION:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_LOITER:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_TAKEOFF:
  case vehicle_status_s::NAVIGATION_STATE_AUTO_VTOL_TAKEOFF:
    vehicle_control_mode.flag_control_auto_enabled = true;
    vehicle_control_mode.flag_control_position_enabled = true;
    vehicle_control_mode.flag_control_velocity_enabled = true;
    vehicle_control_mode.flag_control_altitude_enabled = true;
    vehicle_control_mode.flag_control_climb_rate_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_ACRO:
    vehicle_control_mode.flag_control_manual_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_DESCEND:
    vehicle_control_mode.flag_control_auto_enabled = true;
    vehicle_control_mode.flag_control_climb_rate_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_TERMINATION:
    /* disable all controllers on termination */
    vehicle_control_mode.flag_control_termination_enabled = true;
    break;

  case vehicle_status_s::NAVIGATION_STATE_OFFBOARD:
    vehicle_control_mode.flag_control_offboard_enabled = true;

    if (offboard_control_mode.position) {
      vehicle_control_mode.flag_control_position_enabled = true;
      vehicle_control_mode.flag_control_velocity_enabled = true;
      vehicle_control_mode.flag_control_altitude_enabled = true;
      vehicle_control_mode.flag_control_climb_rate_enabled = true;
      vehicle_control_mode.flag_control_acceleration_enabled = true;
      vehicle_control_mode.flag_control_attitude_enabled = true;
      vehicle_control_mode.flag_control_rates_enabled = true;
      vehicle_control_mode.flag_control_allocation_enabled = true;

    } else if (offboard_control_mode.velocity) {
      vehicle_control_mode.flag_control_velocity_enabled = true;
      vehicle_control_mode.flag_control_altitude_enabled = true;
      vehicle_control_mode.flag_control_climb_rate_enabled = true;
      vehicle_control_mode.flag_control_acceleration_enabled = true;
      vehicle_control_mode.flag_control_attitude_enabled = true;
      vehicle_control_mode.flag_control_rates_enabled = true;
      vehicle_control_mode.flag_control_allocation_enabled = true;

    } else if (offboard_control_mode.acceleration) {
      vehicle_control_mode.flag_control_acceleration_enabled = true;
      vehicle_control_mode.flag_control_attitude_enabled = true;
      vehicle_control_mode.flag_control_rates_enabled = true;
      vehicle_control_mode.flag_control_allocation_enabled = true;

    } else if (offboard_control_mode.attitude) {
      vehicle_control_mode.flag_control_attitude_enabled = true;
      vehicle_control_mode.flag_control_rates_enabled = true;
      vehicle_control_mode.flag_control_allocation_enabled = true;

    } else if (offboard_control_mode.body_rate) {
      vehicle_control_mode.flag_control_rates_enabled = true;
      vehicle_control_mode.flag_control_allocation_enabled = true;

    } else if (offboard_control_mode.thrust_and_torque) {
      vehicle_control_mode.flag_control_allocation_enabled = true;
    }

    break;

  case vehicle_status_s::NAVIGATION_STATE_AUTO_FOLLOW_TARGET:

  // Follow Target supports RC adjustment, so disable auto control mode to disable
  // the Flight Task from exiting itself when RC stick movement is detected.
  case vehicle_status_s::NAVIGATION_STATE_ORBIT:
    vehicle_control_mode.flag_control_manual_enabled = false;
    vehicle_control_mode.flag_control_auto_enabled = false;
    vehicle_control_mode.flag_control_position_enabled = true;
    vehicle_control_mode.flag_control_velocity_enabled = true;
    vehicle_control_mode.flag_control_altitude_enabled = true;
    vehicle_control_mode.flag_control_climb_rate_enabled = true;
    vehicle_control_mode.flag_control_attitude_enabled = true;
    vehicle_control_mode.flag_control_rates_enabled = true;
    vehicle_control_mode.flag_control_allocation_enabled = true;
    break;

  // vehicle_status_s::NAVIGATION_STATE_EXTERNALx: handled in ModeManagement
  default:
    break;
  }
}
```

函数按照当前的`vehicle_status_s`与无人机类型，决定`vehicle_control_mode`控制器使能。值得注意的是，对于旋翼无人机，只有位置控制器在手动操作时可以失能，其它的控制器，包括控制分配器在任何时候都必须使能.
