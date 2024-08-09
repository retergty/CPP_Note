# vehicle_control_mode

表示无人机当前使能的控制器，控制器直接使用这个消息来决定是否运行对应的控制器。

```CPP
struct vehicle_control_mode_s {
  uint64_t timestamp;
  bool flag_armed;
  bool flag_multicopter_position_control_enabled;
  bool flag_control_manual_enabled;
  bool flag_control_auto_enabled;
  bool flag_control_offboard_enabled;
  bool flag_control_position_enabled;
  bool flag_control_velocity_enabled;
  bool flag_control_altitude_enabled;
  bool flag_control_climb_rate_enabled;
  bool flag_control_acceleration_enabled;
  bool flag_control_attitude_enabled;
  bool flag_control_rates_enabled;
  bool flag_control_allocation_enabled;
  bool flag_control_termination_enabled;
  uint8_t source_id;
  uint8_t _padding0[1]; // required for logger
};
```

* 由`commander`模块使用，用来决定当前使能的控制器,它使用消息`vehicle_status`中的`nva_status`字段来决定当前使能的控制器。
* 对于多旋翼无人机来说，只要`flag_control_position_enabled`,`flag_control_velocity_enabled`,`flag_control_altitude_enabled`,`flag_control_climb_rate_enabled`,`flag_control_acceleration_enabled`.作用是相同的，都是使能`flag_multicopter_position_control_enabled`.