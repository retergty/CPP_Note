# takeoff_status

表示无人机起飞时的状态，其它模块使用这个信息决定当前无人机的行为。

```CPP
takeoff_status_s {
  uint64_t timestamp;
  float tilt_limit;
  uint8_t takeoff_state;
  uint8_t _padding0[3]; // required for logger
}
```

## takeoff_state

```CPP
static constexpr uint8_t TAKEOFF_STATE_UNINITIALIZED = 0;
static constexpr uint8_t TAKEOFF_STATE_DISARMED = 1;
static constexpr uint8_t TAKEOFF_STATE_SPOOLUP = 2;
static constexpr uint8_t TAKEOFF_STATE_READY_FOR_TAKEOFF = 3;
static constexpr uint8_t TAKEOFF_STATE_RAMPUP = 4;
static constexpr uint8_t TAKEOFF_STATE_FLIGHT = 5;
```

* 由`flight_mode_manager`模块使用，只有在飞机真正起飞后，才执行飞行任务，否则总是`reactive`飞行任务.
