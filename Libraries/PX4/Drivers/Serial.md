# Serial

不同于`SPI/I2C`的驱动，`Serial`是`nuttx`直接支持的外设，使用`Serial`时更加方便.

## 工作队列

同一个串口的应用都使用在同一个工作队列里.一个串口占有一个工作队列

```CPP
static constexpr wq_config_t ttyS0{"wq:ttyS0", 1728, -21};
static constexpr wq_config_t ttyS1{"wq:ttyS1", 1728, -22};
static constexpr wq_config_t ttyS2{"wq:ttyS2", 1728, -23};
static constexpr wq_config_t ttyS3{"wq:ttyS3", 1728, -24};
static constexpr wq_config_t ttyS4{"wq:ttyS4", 1728, -25};
static constexpr wq_config_t ttyS5{"wq:ttyS5", 1728, -26};
static constexpr wq_config_t ttyS6{"wq:ttyS6", 1728, -27};
static constexpr wq_config_t ttyS7{"wq:ttyS7", 1728, -28};
static constexpr wq_config_t ttyS8{"wq:ttyS8", 1728, -29};
static constexpr wq_config_t ttyS9{"wq:ttyS9", 1728, -30};
static constexpr wq_config_t ttyACM0{"wq:ttyACM0", 1728, -31};
```

其中`ttyACM0`是通用串行总线`USB`的工作队列.

## 文件操作

对串口的操作就是`POSIX`对文件的操作.使用`read`与`write`等标准函数.

## Serial类

`PX4`定义了一个`Serial`类，可以方便地操作串口，同时具有`RAII`的能力.

位于`platforms/common/include/px4_platform_common/Serial.hpp`

```CPP
class Serial
{
public:
 Serial();
 Serial(const char *port, uint32_t baudrate = 57600,
        ByteSize bytesize = ByteSize::EightBits, Parity parity = Parity::None,
        StopBits stopbits = StopBits::One, FlowControl flowcontrol = FlowControl::Disabled);
 virtual ~Serial();

 // Open sets up the port and gets it configured based on desired configuration
 // The port is always opened in NON BLOCKING mode.
 bool open();
 bool isOpen() const;

 bool close();

 ssize_t read(uint8_t *buffer, size_t buffer_size);
 ssize_t readAtLeast(uint8_t *buffer, size_t buffer_size, size_t character_count = 1, uint32_t timeout_ms = 0);

 ssize_t write(const void *buffer, size_t buffer_size);

 void flush();

 // If port is already open then the following configuration functions
 // will reconfigure the port. If the port is not yet open then they will
 // simply store the configuration in preparation for the port to be opened.

 uint32_t getBaudrate() const;
 bool setBaudrate(uint32_t baudrate);

 ByteSize getBytesize() const;
 bool setBytesize(ByteSize bytesize);

 Parity getParity() const;
 bool setParity(Parity parity);

 StopBits getStopbits() const;
 bool setStopbits(StopBits stopbits);

 FlowControl getFlowcontrol() const;
 bool setFlowcontrol(FlowControl flowcontrol);

 bool getSingleWireMode() const;
 bool setSingleWireMode();

 bool getSwapRxTxMode() const;
 bool setSwapRxTxMode();

 bool getInvertedMode() const;
 bool setInvertedMode(bool enable);

 static bool validatePort(const char *port);
 bool setPort(const char *port);
 const char *getPort() const;

private:
 // Disable copy constructors
 Serial(const Serial &);
 Serial &operator=(const Serial &);

 // platform implementation
 SerialImpl _impl;
};
```

## 例子

以自定义的`IMUForward`类，将`vehicle_imu`从串口转发出去.

```CPP
class IMUForward : public px4::ScheduledWorkItem
{
public:
 IMUForward(const char *port);
 ~IMUForward() override;
 int init();
 void print_info();
private:
 void Run() override;
 void stop();
 void FillUpImuStream(const vehicle_imu_s &imu);

 device::Serial  _uart {};
 char _port[20] {};
 uORB::SubscriptionCallbackWorkItem _vehicle_imu_sub{this, ORB_ID(vehicle_imu)};
 vehicle_imu_s _vehicle_imu;
 ImuStream _imu_stream;

 perf_counter_t _sample_perf;
};
```

* 必须继承`ScheduledWorkItem`，是一个工作队列类.
* 等待`vehicle_imu`消息，到达时就会立即转发.

### module.yaml

```CPP
module_name: IMU FORWARD
serial_config:

    - command: imu_forward start -d ${SERIAL_DEV}
      port_config_param:
        name: IMU_FOR_CONFIG
        group: Serial
```

当`IMU_FOR_CONFIG`被设置时，自动运行命令`imu_forward start -d ${SERIAL_DEV}`，传递`${SERIAL_DEV}`要运行在的串口.

### kconfig

```CPP
menuconfig DRIVERS_IMU_FORWARD
 bool "imu_forward"
 default n
 ---help---
  Enable support for imu_forward
```

### 发送数据

```CPP
void IMUForward::Run()
{
 perf_begin(_sample_perf);

 if (!_uart.isOpen()) {
  // Configure UART port
  if (!_uart.setPort(_port)) {
   PX4_ERR("Error configuring serial device on port %s", _port);
   return;
  }

  // Configure the desired baudrate if one was specified by the user.
  // Otherwise the default baudrate will be used.
  if (! _uart.setBaudrate(921600U)) {
   PX4_ERR("Error setting baudrate to %u on %s", 921600U, _port);
   return;
  }

  // Open the UART. If this is successful then the UART is ready to use.
  if (! _uart.open()) {
   PX4_ERR("Error opening serial device  %s", _port);
   return;
  }
 }

 if (_uart.isOpen()) {
  if (_vehicle_imu_sub.update(&_vehicle_imu)) {
   FillUpImuStream(_vehicle_imu);
   size_t sended = _uart.write(&_imu_stream, sizeof(_imu_stream));

   if (sended != sizeof(_imu_stream)) {
    PX4_ERR("Error in forwarding imu, Sended %u, expected %u ", sended, sizeof(_imu_stream));
   }
  }
 }

 perf_end(_sample_perf);
}
```

打开指定串口，写入数据.
