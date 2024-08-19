# VehicleThrustSetpoint

```msg
uint64 timestamp        # time since system start (microseconds)
uint64 timestamp_sample # timestamp of the data sample on which this message is based (microseconds)

float32[3] xyz          # thrust setpoint along X, Y, Z body axis [-1, 1]

# TOPICS vehicle_thrust_setpoint
# TOPICS vehicle_thrust_setpoint_virtual_fw vehicle_thrust_setpoint_virtual_mc
```

* 由`mc_rate_control`发布，表示推力`setpoint`已经归一化，归一化方法为使用推力估计，即能产生的多少倍的重力。
* 由`control_allocatoe`接收，用来进行控制分配。
