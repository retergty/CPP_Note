# trajectory_setpoint

表示无人机航迹的`setpoint`.

```CPP
struct __EXPORT trajectory_setpoint_s {
  uint64_t timestamp;
  float position[3];
  float velocity[3];
  float acceleration[3];
  float jerk[3];
  float yaw;
  float yawspeed;
};
```

* 由`flight_mode_manager`或者远程电脑发布
* 由`mc_pos_control`使用，作为期望三轴位置。