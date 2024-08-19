# VehicleTorqueSetpoint

```msg
uint64 timestamp        # time since system start (microseconds)
uint64 timestamp_sample # timestamp of the data sample on which this message is based (microseconds)

float32[3] xyz          # torque setpoint about X, Y, Z body axis (normalized)

# TOPICS vehicle_torque_setpoint
# TOPICS vehicle_torque_setpoint_virtual_fw vehicle_torque_setpoint_virtual_mc
```

* 由`mc_rate_control`发布，表示力矩`setpoint`已经归一化。
* 由`control_allocatoe`接收，用来进行控制分配。