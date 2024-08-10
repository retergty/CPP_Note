# flight_mode_manager

`flight_mode_manager`模块管理飞行任务，它按照当前的飞行模式以及参数设置，来决定选择的飞行任务与发布的`setpoint`.

这个模块只会在无人机真正起飞时才会运行飞行任务，起飞操作不是由这个模块维护的。

## 关键数据流

### 订阅

#### `vehicle_local_position`

这也是这个模块的驱动订阅，表示当前无人机的三维位置，速度，加速度等信息，由`EKF`模块发布。

#### `vehicle_status`

表示当前无人机的状态，该模块主要使用`vehicle_status.nav_state`字段，用这个字段来选择飞行任务。

#### `takeoff_status`

表示当前无人机的起飞状态，只有无人机真正起飞时才会运行飞行任务。

### 发布

#### `trajectory_setpoint`

表示无人机的航迹`setpoint`,由当前的飞行任务修改，传递给`mc_pos_control`,只有当`position`控制器使能时才有效果.

#### `vehicle_constraints`

表示无人机的限制，有当前飞行任务修改，传递给控制器。

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

## 代码分析

### Run函数

`Run`函数比较简单，进行消息的接收以及调用两个主要函数,`start_flight_task`与`generateTrajectorySetpoint`.

### start_flight_task函数

`start_flight_task`函数执行切换任务的功能，它根据`vehicle_status.nav_state`以及参数`MPC_POS_MODE`决定具体执行的飞行任务。注意，它也可以选择不执行飞行任务，这种情况下，由外部电脑或者是遥控器发送命令。

### generateTrajectorySetpoint函数

这个函数实际生成`trajectory_setpoint`与`vehicle_constraints`消息并发布，只有在当前有飞行任务的情况下才会调用。

```CPP
if (_takeoff_state < takeoff_status_s::TAKEOFF_STATE_RAMPUP) {
  // reactivate the task which will reset the setpoint to current state
  current_task.task->reActivate();
}
```

如果当前飞机没有起飞，那么会一直重置飞行任务，不会真正执行飞行任务。
