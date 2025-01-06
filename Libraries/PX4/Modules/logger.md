# logger

`logger`模块管理日志，它实现的功能如下

* 按照指定的速率，订阅一系列消息，并将其写入到存储设备`sd`卡中去.
* 可以选择写入到`sd`卡里的消息主题.

## logger类

```CPP
class Logger : public ModuleBase<Logger>, public ModuleParams
```

`logger`类是独立线程的模块.

### 关键参数

#### `SDLOG_PROFILE`

```CPP
/**
 * Logging topic profile (integer bitmask).
 *
 * This integer bitmask controls the set and rates of logged topics.
 * The default allows for general log analysis while keeping the
 * log file size reasonably small.
 *
 * Enabling multiple sets leads to higher bandwidth requirements and larger log
 * files.
 *
 * Set bits true to enable:
 * 0 : Default set (used for general log analysis)
 * 1 : Full rate estimator (EKF2) replay topics
 * 2 : Topics for thermal calibration (high rate raw IMU and Baro sensor data)
 * 3 : Topics for system identification (high rate actuator control and IMU data)
 * 4 : Full rates for analysis of fast maneuvers (RC, attitude, rates and actuators)
 * 5 : Debugging topics (debug_*.msg topics, for custom code)
 * 6 : Topics for sensor comparison (low rate raw IMU, Baro and magnetometer data)
 * 7 : Topics for computer vision and collision avoidance
 * 8 : Raw FIFO high-rate IMU (Gyro)
 * 9 : Raw FIFO high-rate IMU (Accel)
 * 10: Logging of mavlink tunnel message (useful for payload communication debugging)
 *
 * @min 0
 * @max 2047
 * @bit 0 Default set (general log analysis)
 * @bit 1 Estimator replay (EKF2)
 * @bit 2 Thermal calibration
 * @bit 3 System identification
 * @bit 4 High rate
 * @bit 5 Debug
 * @bit 6 Sensor comparison
 * @bit 7 Computer Vision and Avoidance
 * @bit 8 Raw FIFO high-rate IMU (Gyro)
 * @bit 9 Raw FIFO high-rate IMU (Accel)
 * @bit 10 Mavlink tunnel message logging
 * @reboot_required true
 * @group SD Logging
 */
PARAM_DEFINE_INT32(SDLOG_PROFILE, 1);
```

是一个比特位，预先定义了一系列消息组，每个消息组实现一组功能，可以按照需要使用.

### 代码分析

#### 管理`SDLOG_PROFILE`定义的`log`模式具体主题

```CPP

void LoggedTopics::add_high_rate_topics()
{
	// maximum rate to analyze fast maneuvers (e.g. for racing)
	add_topic("manual_control_setpoint");
	add_topic_multi("rate_ctrl_status", 20, 2);
	add_topic("sensor_combined");
	add_topic("vehicle_angular_velocity");
	add_topic("vehicle_attitude");
	add_topic("vehicle_attitude_setpoint");
	add_topic("vehicle_rates_setpoint");

	add_topic("esc_status", 5);
	add_topic("actuator_motors");
	add_topic("actuator_outputs_debug");
	add_topic("actuator_servos");
	add_topic_multi("vehicle_thrust_setpoint", 0, 2);
	add_topic_multi("vehicle_torque_setpoint", 0, 2);
}
```

`add_high_rate_topics`就是`SDLOG_PROFILE`中`bit 4`的`log`类型。

#### 实际添加主题

```CPP
bool LoggedTopics::add_topic(const char *name, uint16_t interval_ms, uint8_t instance, bool optional)
{
	interval_ms /= _rate_factor;

	const orb_metadata *const *topics = orb_get_topics();
	bool success = false;

	for (size_t i = 0; i < orb_topics_count(); i++) {
		if (strcmp(name, topics[i]->o_name) == 0) {
			bool already_added = false;

			// check if already added: if so, only update the interval
			for (int j = 0; j < _subscriptions.count; ++j) {
				if (_subscriptions.sub[j].id == static_cast<ORB_ID>(topics[i]->o_id) &&
				    _subscriptions.sub[j].instance == instance) {

					PX4_DEBUG("logging topic %s(%" PRIu8 "), interval: %" PRIu16 ", already added, only setting interval",
						  topics[i]->o_name, instance, interval_ms);

					_subscriptions.sub[j].interval_ms = interval_ms;
					success = true;
					already_added = true;
					break;
				}
			}

			if (!already_added) {
				success = add_topic(topics[i], interval_ms, instance, optional);

				if (success) {
					PX4_DEBUG("logging topic: %s(%" PRIu8 "), interval: %" PRIu16, topics[i]->o_name, instance, interval_ms);
				}

				break;
			}
		}
	}

	return success;
}
```

`add_topic`函数管理添加的主题。实际上是管理了一个`_subscriptions.sub[j]`数组，这个数组就是要订阅的主题，以及期望订阅的速率.
