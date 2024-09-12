# flight_mode_manager

`flight_mode_manager`模块管理飞行任务，它按照当前的飞行模式以及参数设置，来决定选择的飞行任务与发布的`setpoint`.

这个模块只会在无人机真正起飞时才会运行飞行任务，起飞状态不是由这个模块维护的，但是自动起飞`trajectory_setpoint`是有这个模块生成的.

它实现的功能如下

* 在自动飞行模式下，执行`AUTO`类型飞行任务，生成`trajectory_setpoint`与`vehicle_constraints`管理自动飞行.
* 在辅助飞行模式下，接收手柄`manual_control_setpoint`，执行`Manual`类型飞行任务，生成`trajectory_setpoint`与`vehicle_constraints`管理辅助飞行.
* 在发生紧急情况或者任务失败时，执行安全类型飞行任务，生成`trajectory_setpoint`与`vehicle_constraints`管理安全保护.
* 发布`trajectory_setpoint`给`mc_pos_control`控制器模块作为期望输入.
* 发布`vehicle_constraints`给`mc_pos_control`控制器模块管理飞行限制与是否起飞.

## 关键数据流

### 订阅

#### `vehicle_local_position`

这是这个模块的驱动订阅，表示当前无人机的三维位置，速度，加速度等信息，由`EKF`模块发布。

#### `vehicle_status`

表示当前无人机的状态，该模块主要使用`vehicle_status.nav_state`字段，用这个字段来选择飞行任务。

#### `takeoff_status`

表示当前无人机的起飞状态，只有无人机真正起飞时才会运行飞行任务。

#### `manual_control_setpoint`

表示无人机的手柄操作的输入`setpoint`，由`manual_control`发布，在辅助飞行模式下使用。具体是在`FlightTaskManualAltitude`类下的`Sticks`成员变量里订阅的。

#### `vehicle_control_mode_s`

表示无人机当前使能的控制模式，使用这个消息决定飞行任务.

### 发布

#### `trajectory_setpoint`

表示无人机的航迹`setpoint`,由当前的飞行任务修改，传递给`mc_pos_control`,只有当`position`控制器使能时才有效果,但是所有飞行任务都是在`position`控制器使能的情况下运行的，如果`position`控制器不使能，意味着当前是手动模式，直接由`mc_att_control`控制器处理手动输入`manual_control_setpoint`，不会运行任何飞行任务.

这是这个模块最重要的发布消息，

#### `vehicle_constraints`

表示无人机的限制，上升，下降的速度，由当前飞行任务修改，以及是否想要起飞，传递给控制器。

## FlightModeManager类

```CPP
class FlightModeManager : public ModuleBase<FlightModeManager>, public ModuleParams, public px4::WorkItem
```

`FlightModeManager`类是在`nav_and_controllers`的工作队列中的模块。

### 关键成员变量

```CPP
struct flight_task_t {
  FlightTask *task{nullptr};
  FlightTaskIndex index{FlightTaskIndex::None};
} _current_task{};
```

指向当前的任务,如果`task`为`nullptr`，意味着当前没有飞行任务，意味着以下几种情况之一

* 无人机处于没有飞行任务的模式，比如`offboard`模式，由机载电脑直接发送`trajectory_setpoint`.
* 无人机的`position`控制失能，无人机处于手动模式下，由遥控器直接控制姿态。注意，不是所有使用了遥控器的飞行模式都没有飞行任务，在辅助飞行模式下也有为了保持当前位置的飞行任务。

此外，用户可以自行添加飞行任务。

### 关键成员函数

#### Run函数

`Run`函数比较简单，进行消息的接收以及调用两个主要函数,`start_flight_task`与`generateTrajectorySetpoint`.

#### start_flight_task函数

`start_flight_task`函数执行切换任务的功能，它根据`vehicle_status.nav_state`以及参数`MPC_POS_MODE`决定具体执行的飞行任务。注意，它也可以选择不执行飞行任务，这种情况下，由外部电脑或者是遥控器发送命令。

#### generateTrajectorySetpoint函数

这个函数实际生成`trajectory_setpoint`与`vehicle_constraints`消息并发布，只有在当前有飞行任务的情况下才会调用。

```CPP
if (_takeoff_state < takeoff_status_s::TAKEOFF_STATE_RAMPUP) {
  // reactivate the task which will reset the setpoint to current state
  current_task.task->reActivate();
}
```

如果当前飞机没有起飞，那么会一直重置飞行任务，不会真正执行飞行任务。

### 代码分析

#### 飞行任务选择

```CPP
void FlightModeManager::start_flight_task()
{
  // Do not run any flight task for VTOLs in fixed-wing mode
  if ((_vehicle_status_sub.get().vehicle_type == vehicle_status_s::VEHICLE_TYPE_FIXED_WING)
      || ((_vehicle_status_sub.get().nav_state >= vehicle_status_s::NAVIGATION_STATE_EXTERNAL1)
    && (_vehicle_status_sub.get().nav_state <= vehicle_status_s::NAVIGATION_STATE_EXTERNAL8))) {
    switchTask(FlightTaskIndex::None);
    return;
  }

  // Only run transition flight task if altitude control is enabled (e.g. in Altitdue, Position, Auto flight mode)
  if (_vehicle_status_sub.get().in_transition_mode && _vehicle_control_mode_sub.get().flag_control_altitude_enabled) {
    switchTask(FlightTaskIndex::Transition);
    return;
  }

  bool found_some_task = false;
  bool matching_task_running = true;
  bool task_failure = false;
  const bool nav_state_descend = (_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_DESCEND);

  // Follow me
  if (_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_AUTO_FOLLOW_TARGET) {
    found_some_task = true;
    FlightTaskError error = FlightTaskError::InvalidTask;

#if !defined(CONSTRAINED_FLASH)
    error = switchTask(FlightTaskIndex::AutoFollowTarget);
#endif // !CONSTRAINED_FLASH

    if (error != FlightTaskError::NoError) {
      matching_task_running = false;
      task_failure = true;
    }
  }

  // Orbit
  if ((_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_ORBIT)
      && !_command_failed) {
    found_some_task = true;
    FlightTaskError error = FlightTaskError::InvalidTask;

#if !defined(CONSTRAINED_FLASH)
    error = switchTask(FlightTaskIndex::Orbit);
#endif // !CONSTRAINED_FLASH

    if (error != FlightTaskError::NoError) {
      matching_task_running = false;
      task_failure = true;
    }
  }

  // Navigator interface for autonomous modes
  if (_vehicle_control_mode_sub.get().flag_control_auto_enabled
      && !nav_state_descend) {
    found_some_task = true;

    if (switchTask(FlightTaskIndex::Auto) != FlightTaskError::NoError) {
      matching_task_running = false;
      task_failure = true;
    }
  }

  // position slow mode
  if (_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_POSITION_SLOW) {
    found_some_task = true;
    FlightTaskError error = switchTask(FlightTaskIndex::ManualAccelerationSlow);
    task_failure = error != FlightTaskError::NoError;
  }

  // Manual position control
  if ((_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_POSCTL) || task_failure) {
    found_some_task = true;
    FlightTaskError error = FlightTaskError::NoError;

    switch (_param_mpc_pos_mode.get()) {
    case 0:
      error = switchTask(FlightTaskIndex::ManualPosition);
      break;

    case 3:
      error = switchTask(FlightTaskIndex::ManualPositionSmoothVel);
      break;
    case 5:
      error = switchTask(FlightTaskIndex::ContinunousYaw);
      break;
    case 4:
    default:
      if (_param_mpc_pos_mode.get() != 4) {
        PX4_ERR("MPC_POS_MODE %" PRId32 " invalid, resetting", _param_mpc_pos_mode.get());
        _param_mpc_pos_mode.set(4);
        _param_mpc_pos_mode.commit();
      }

      error = switchTask(FlightTaskIndex::ManualAcceleration);
      break;
    }

    task_failure = (error != FlightTaskError::NoError);
    matching_task_running = matching_task_running && !task_failure;
  }

  // Manual altitude control
  if ((_vehicle_status_sub.get().nav_state == vehicle_status_s::NAVIGATION_STATE_ALTCTL) || task_failure) {
    found_some_task = true;
    FlightTaskError error = FlightTaskError::NoError;

    switch (_param_mpc_pos_mode.get()) {
    case 0:
      error = switchTask(FlightTaskIndex::ManualAltitude);
      break;

    case 3:
    default:
      error = switchTask(FlightTaskIndex::ManualAltitudeSmoothVel);
      break;
    }

    task_failure = (error != FlightTaskError::NoError);
    matching_task_running = matching_task_running && !task_failure;
  }

  // Emergency descend
  if (nav_state_descend || task_failure) {
    found_some_task = true;

    FlightTaskError error = switchTask(FlightTaskIndex::Descend);

    task_failure = (error != FlightTaskError::NoError);
    matching_task_running = matching_task_running && !task_failure;
  }

  if (task_failure) {
    // For some reason no task was able to start, go into failsafe flighttask
    found_some_task = (switchTask(FlightTaskIndex::Failsafe) == FlightTaskError::NoError);
  }

  if (!found_some_task) {
    switchTask(FlightTaskIndex::None);
  }

  if (!matching_task_running && _vehicle_control_mode_sub.get().flag_armed && !_no_matching_task_error_printed) {
    PX4_ERR("Matching flight task was not able to run, Nav state: %" PRIu8 ", Task: %" PRIu32,
      _vehicle_status_sub.get().nav_state, static_cast<uint32_t>(_current_task.index));
  }

  _no_matching_task_error_printed = !matching_task_running;
}
```

飞行任务选择由`start_flight_task`决定，借助`vehicle_status.nav_state`，`vehicle_control_mode_s`,`px4::params::MPC_POS_MODE`三者共同决定使用的飞行任务.事实上，在`commander`模块里，`vehicle_control_mode_s`由`vehicle_status.nav_state`决定.

由于多旋翼无人机不可能处于`in_transition_mode`,所以消息`_vehicle_status_sub`没有效果.

#### 自动飞行任务

飞行任务`AUTO`管理自动飞行,最常见的自动飞行任务就是`takeoff`与`land`。比如，通过`commander takeoff`命令，会切换当前飞行任务为`AUTO`,这个飞行任务生成适合起飞的`setpoint`与`constraints`,把`constraints.want_takeoff`设置为真.

#### 辅助飞行任务

除了飞行任务`AUTO`外，其余的飞行任务有许多都是辅助飞行任务，意味着他们接收`manual_control_setpoint`,生成`trajectory_setpoint`，并在其中应用限制，比如保持高度，或者是减慢加速度.

#### 其余飞行任务

其余飞行任务都表示一个特定的飞行动作，比如`Orbit`表示旋转飞行，这也是通过逐渐生成`trajectory_setpoint`实现的.
