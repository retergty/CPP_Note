# pwm_out

`pwm_out`位于`src/drivers/pwm_out`中，实现的功能如下.

* 将期望控制转化为`PWM`信号并输出，控制电机与舵机
* 沟通硬件层，初始化定时器，更新定时器的值.

## 关键数据流

```CPP
MixingOutput _mixing_output{PARAM_PREFIX, DIRECT_PWM_OUTPUT_CHANNELS, *this, MixingOutput::SchedulingPolicy::Auto, true};
```

### 订阅

#### `actuator_motors`

电机期望速度.

#### `actuator_servos`

舵机期望位置.

## `pwm_out`类

### `updateOutputs`函数

这个函数更新pwm值.

## 代码分析

### 更新PWM值

```CPP
bool PWMOut::updateOutputs(bool stop_motors, uint16_t outputs[MAX_ACTUATORS],
      unsigned num_outputs, unsigned num_control_groups_updated)
{
 /* output to the servos */
 if (_pwm_initialized) {
  for (size_t i = 0; i < num_outputs; i++) {
   if (!_mixing_output.isFunctionSet(i)) {
    // do not run any signal on disabled channels
    outputs[i] = 0;
   }

   if (_pwm_mask & (1 << i)) {
    up_pwm_servo_set(i, outputs[i]);
   }
  }
 }

 /* Trigger all timer's channels in Oneshot mode to fire
  * the oneshots with updated values.
  */
 if (num_control_groups_updated > 0) {
  up_pwm_update(_pwm_mask);
 }

 return true;
}
```

接受`output`输出值,并转化为PWM值.

此时，`output`值已经是PWM值范围了，从`800`到`2200`，按照`px4`规定，表示高电平时间`0.8ms`到`2.2`毫秒。

### 初始化定时器

```CPP
bool PWMOut::update_pwm_out_state(bool on)
{
 if (on && !_pwm_initialized && _pwm_mask != 0) {

  for (int timer = 0; timer < MAX_IO_TIMERS; ++timer) {
   _timer_rates[timer] = -1;

   uint32_t channels = io_timer_get_group(timer);

   if (channels == 0) {
    continue;
   }

   char param_name[17];
   snprintf(param_name, sizeof(param_name), "%s_TIM%u", _mixing_output.paramPrefix(), timer);

   int32_t tim_config = 0;
   param_t handle = param_find(param_name);
   param_get(handle, &tim_config);

   if (tim_config > 0) {
    _timer_rates[timer] = tim_config;

   } else if (tim_config == -1) { // OneShot
    _timer_rates[timer] = 0;

   } else {
    _pwm_mask &= ~channels; // don't use for pwm
   }
  }

  int ret = up_pwm_servo_init(_pwm_mask);

  if (ret < 0) {
   PX4_ERR("up_pwm_servo_init failed (%i)", ret);
   return false;
  }

  _pwm_mask = ret;

  // set the timer rates
  for (int timer = 0; timer < MAX_IO_TIMERS; ++timer) {
   uint32_t channels = _pwm_mask & up_pwm_servo_get_rate_group(timer);

   if (channels == 0) {
    continue;
   }

   ret = up_pwm_servo_set_rate_group_update(timer, _timer_rates[timer]);

   if (ret != 0) {
    PX4_ERR("up_pwm_servo_set_rate_group_update failed for timer %i, rate %i (%i)", timer, _timer_rates[timer], ret);
    _timer_rates[timer] = -1;
    _pwm_mask &= ~channels;
   }
  }

  _pwm_initialized = true;

  // disable unused functions
  for (unsigned i = 0; i < _num_outputs; ++i) {
   if (((1 << i) & _pwm_mask) == 0) {
    _mixing_output.disableFunction(i);
   }
  }
 }

 up_pwm_servo_arm(on, _pwm_mask);
 return true;
}
```

```CPP
int up_pwm_servo_init(uint32_t channel_mask)
{
 /* Init channels */
 uint32_t current = io_timer_get_mode_channels(IOTimerChanMode_PWMOut) |
      io_timer_get_mode_channels(IOTimerChanMode_OneShot);

 /* First free the current set of PWMs */

 for (unsigned channel = 0; current != 0 &&  channel < MAX_TIMER_IO_CHANNELS; channel++) {
  if (current & (1 << channel)) {
   io_timer_set_enable(false, IOTimerChanMode_PWMOut, 1 << channel);
   io_timer_unallocate_channel(channel);
   current &= ~(1 << channel);
  }
 }


 /* Now allocate the new set */

 int ret_val = OK;
 int channels_init_mask = 0;

 for (unsigned channel = 0; channel_mask != 0 &&  channel < MAX_TIMER_IO_CHANNELS; channel++) {
  if (channel_mask & (1 << channel)) {

   /* OneShot is set later, with the set_rate_group_update call. Init to PWM mode for now */

   ret_val = io_timer_channel_init(channel, IOTimerChanMode_PWMOut, NULL, NULL);
   channel_mask &= ~(1 << channel);

   if (OK == ret_val) {
    channels_init_mask |= 1 << channel;

   } else if (ret_val == -EBUSY) {
    /* either timer or channel already used - this is not fatal */
    ret_val = 0;
   }
  }
 }

 return ret_val == OK ? channels_init_mask : ret_val;
}
```

`up_pwm_servo_init`函数与硬件关联，会把`PWM`信号初始化为`50Hz`,且定时器频率是`1MHz`，时间`1e-6s`,`ARR`寄存器是`19999`也就是周期`20ms`，`PWM`信号`50Hz`.`CRR`寄存器正好就是`output`的值.
