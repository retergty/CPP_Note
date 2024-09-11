# vehicle_status

这是最重要的一个消息，保存着无人机当前的飞行状态。比如导航状态，导航模式，起飞时间等。

```CPP
vehicle_status_s {
  uint64_t timestamp;
  uint64_t armed_time; //无人机准备完成时间
  uint64_t takeoff_time; // 无人机起飞时间
  uint64_t nav_state_timestamp;
  uint32_t valid_nav_states_mask;
  uint32_t can_set_nav_states_mask;
  uint16_t failure_detector_status;
  uint8_t arming_state;
  uint8_t latest_arming_reason;
  uint8_t latest_disarming_reason;
  uint8_t nav_state_user_intention;
  uint8_t nav_state; //导航状态
  uint8_t executor_in_charge;
  uint8_t hil_state;
  uint8_t vehicle_type;
  bool failsafe;
  bool failsafe_and_user_took_over;
  uint8_t failsafe_defer_state;
  bool gcs_connection_lost;
  uint8_t gcs_connection_lost_counter;
  bool high_latency_data_link_lost;
  bool is_vtol;
  bool is_vtol_tailsitter;
  bool in_transition_mode;
  bool in_transition_to_fw;
  uint8_t system_type;
  uint8_t system_id;
  uint8_t component_id;
  bool safety_button_available;
  bool safety_off;
  bool power_input_valid;
  bool usb_connected;
  bool open_drone_id_system_present;
  bool open_drone_id_system_healthy;
  bool parachute_system_present;
  bool parachute_system_healthy;
  bool avoidance_system_required;
  bool avoidance_system_valid;
  bool rc_calibration_in_progress;
  bool calibration_enabled;
  bool pre_flight_checks_pass;
  uint8_t _padding0[4]; // required for logger
}
```

## nav_state

```CPP
static constexpr uint8_t NAVIGATION_STATE_MANUAL = 0;
static constexpr uint8_t NAVIGATION_STATE_ALTCTL = 1;
static constexpr uint8_t NAVIGATION_STATE_POSCTL = 2;
static constexpr uint8_t NAVIGATION_STATE_AUTO_MISSION = 3;
static constexpr uint8_t NAVIGATION_STATE_AUTO_LOITER = 4;
static constexpr uint8_t NAVIGATION_STATE_AUTO_RTL = 5;
static constexpr uint8_t NAVIGATION_STATE_POSITION_SLOW = 6;
static constexpr uint8_t NAVIGATION_STATE_FREE5 = 7;
static constexpr uint8_t NAVIGATION_STATE_FREE4 = 8;
static constexpr uint8_t NAVIGATION_STATE_FREE3 = 9;
static constexpr uint8_t NAVIGATION_STATE_ACRO = 10;
static constexpr uint8_t NAVIGATION_STATE_FREE2 = 11;
static constexpr uint8_t NAVIGATION_STATE_DESCEND = 12;
static constexpr uint8_t NAVIGATION_STATE_TERMINATION = 13;
static constexpr uint8_t NAVIGATION_STATE_OFFBOARD = 14;
static constexpr uint8_t NAVIGATION_STATE_STAB = 15;
static constexpr uint8_t NAVIGATION_STATE_FREE1 = 16;
static constexpr uint8_t NAVIGATION_STATE_AUTO_TAKEOFF = 17;
static constexpr uint8_t NAVIGATION_STATE_AUTO_LAND = 18;
static constexpr uint8_t NAVIGATION_STATE_AUTO_FOLLOW_TARGET = 19;
static constexpr uint8_t NAVIGATION_STATE_AUTO_PRECLAND = 20;
static constexpr uint8_t NAVIGATION_STATE_ORBIT = 21;
static constexpr uint8_t NAVIGATION_STATE_AUTO_VTOL_TAKEOFF = 22;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL1 = 23;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL2 = 24;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL3 = 25;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL4 = 26;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL5 = 27;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL6 = 28;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL7 = 29;
static constexpr uint8_t NAVIGATION_STATE_EXTERNAL8 = 30;
static constexpr uint8_t NAVIGATION_STATE_MAX = 31;
```

* 由`flight_mode_manager`模块使用，用来决定当前的飞行任务。
* 由`commander`模块发布，决定当前使能的控制器，`vehicle_control_mode_s`
