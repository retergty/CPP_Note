# 飞行任务

飞行任务(Flight Task)是上层的飞行任务，用户使用飞行任务指示无人机要飞行到的目的地(setpoint).

`PX4`提供了一个`FlightTask`框架类，继承这个类便可以获取到当前无人机的飞行状态,以及设置`setpoint`，此外，还可以递归的继承从`FligtTask`继承的类，这样便可以在一个已有的飞行任务上修改。

`PX4`支持动态切换飞行任务。

## 框架

在`v1.14`版本中，所有的飞行任务都是由模块`flight_mode_manager`进行管理的。也就是说，所有的飞行任务运行在同一个线程，同一时间内只能有一个飞行任务运行。

所有的飞行任务类都具有前缀`FlightTask`.并都处于`tasks`子文件夹下。

## FlightTask类

`FlightTask`类是飞行任务的通用基类，它提供了一个统一的框架，同时自动地订阅当前的位置信息。

### setpoint

```CPP
  /**
   * Setpoints which the position controller has to execute.
   * Setpoints that are set to NAN are not controlled. Not all setpoints can be set at the same time.
   * If more than one type of setpoint is set, then order of control is a as follow: position, velocity,
   * acceleration, thrust. The exception is _position_setpoint together with _velocity_setpoint, where the
   * _velocity_setpoint and _acceleration_setpoint are used as feedforward.
   * _jerk_setpoint does not executed but just serves as internal state.
   */
  matrix::Vector3f _position_setpoint;
  matrix::Vector3f _velocity_setpoint;
  matrix::Vector3f _velocity_setpoint_feedback;
  matrix::Vector3f _acceleration_setpoint;
  matrix::Vector3f _acceleration_setpoint_feedback;
  matrix::Vector3f _jerk_setpoint;
```

`FlightTask`的保护成员变量，控制器接受这些`setpoint`并控制无人机满足这个`setpoint`.

如果一个`setpoint`设置为`NAN`，意味着控制器不需要控制这个`setpoint`.

如果多于一种`setpoint`类型被设置，那么就会按照`position`,`velocity`,`acceleration`,`thrust`顺序控制。

只会在`FlightTask::activate()`函数内被清空为`NAN`.

### 当前状态

```CPP
  /* Current vehicle state */
  matrix::Vector3f _position; /**< current vehicle position */
  matrix::Vector3f _velocity; /**< current vehicle velocity */
```

这两个保护成员变量存储的当前的无人机状态信息，`FlightTask`类自动地订阅当前无人机状态信息。保证每次`update`调用时，无人机状态信息都是最新的。

### constraints

```CPP
vehicle_constraints_s _constraints{};
```

`FlightTask`的保护成员变量，控制器接受这些`constraint`并满足。

只会在`FlightTask::activate()`函数内被设置为默认值.

## 实例

参考文档

* [Flight Tasks](https://docs.px4.io/v1.14/en/concept/flight_tasks.html)

添加一个新的飞行任务的方法如下,以`ContinunousYaw`为例

在`tasks`文件夹下创建一个名为`ContinunousYaw`的子文件夹,这个便是定义的飞行任务名。

```shell
mkdir ContinunousYaw
cd ContinunousYaw
```

创建`FlightTaskContinunousYaw.hpp`.

```CPP
//FlightTaskContinunousYaw.hpp
#pragma once

#include "FlightTask.hpp"

class FlightTaskContinunousYaw : public FlightTask
{
public:
  FlightTaskContinunousYaw() = default;
  virtual ~FlightTaskContinunousYaw() = default;

  bool update() override;
  bool activate(const trajectory_setpoint_s & last_setpoint) override;
private:
  float _origin_z = 0.0f;
};
```

它继承了`FlightTask`,注意飞行任务的`C++`类名与文件名***需要***加上`FlightTask`前缀。

创建`FlightTaskContinunousYaw.cpp`

```CPP
//FlightTaskContinunousYaw.cpp
#include "FlightTaskContinunousYaw.hpp"

constexpr static float yaw_default = 45.0f * 3.142f / 180.0f;
constexpr static float yaw_defalut_inv = -yaw_default;
bool FlightTaskContinunousYaw::activate(const trajectory_setpoint_s &last_setpoint)
{
  bool ret = FlightTask::activate(last_setpoint);

  _position_setpoint(0) = _position(0);
  _position_setpoint(1) = _position(1);

  _origin_z = _position(2);

  _yaw_setpoint = yaw_default;

  _velocity_setpoint(2) = -1.0f;
  return ret;
}

bool FlightTaskContinunousYaw::update()
{
  float diff_z = _position(2) - _origin_z;

  if (diff_z <= -8.0f) {
    _velocity_setpoint(2) = 1.0f;
    _yaw_setpoint = yaw_defalut_inv;

  } else if (diff_z >= 0.0f) {
    _velocity_setpoint(2) = -1.0f;
    _yawspeed_setpoint = yaw_default;
  }

  return true;
}
```

* `activate`函数会在切换到这个任务时调用。
* `update`函数会在控制循环中周期性地调用。

创建`CMakeLists.txt`

```CMake
px4_add_library(FlightTaskContinunousYaw
  FlightTaskContinunousYaw.cpp
)

target_link_libraries(FlightTaskContinunousYaw PUBLIC FlightTask)
target_include_directories(FlightTaskContinunousYaw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

* 添加一个新的库文件`FlightTaskContinunousYaw`
* 库文件依赖于`FlightTask`的头文件，所以把它链接进来

修改`flight_mode_manager`的顶层`CMakeLists.txt`,添加

```CMake
list(APPEND flight_tasks_all
  ContinunousYaw
)
```

* `flight_tasks_all`添加`ContinunousYaw`任务，它会自动地添加子文件夹。

把这个飞行任务放在`POSITION`模式下，修改参数`MPC_POS_MODE`,在`multicopter_position_mode_params.c`中，添加注释

```C
/**
 * Position/Altitude mode variant
 *
 * The supported sub-modes are:
 * 0 Sticks directly map to velocity setpoints without smoothing.
 *   Also applies to vertical direction and Altitude mode.
 *   Useful for velocity control tuning.
 * 3 Sticks map to velocity but with maximum acceleration and jerk limits based on
 *   jerk optimized trajectory generator (different algorithm than 1).
 * 4 Sticks map to acceleration and there's a virtual brake drag
 *
 * @value 0 Direct velocity
 * @value 3 Smoothed velocity
 * @value 4 Acceleration based
 * @value 5 Continunous yaw
 * @group Multicopter Position Control
 */
PARAM_DEFINE_INT32(MPC_POS_MODE, 4);
```

把参数`5`分配到`Continunous yaw`中。

修改`FlightModeManager.cpp`中的代码，添加当`MPC_POS_MODE`为`5`时的逻辑

```CPP
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
```

这样，当飞机起飞时，我们切换为`POSITION MODE`，无人机便会进行我们定义的飞行任务。
