# SPI And I2C

`px4`的`I2C SPI`架构驱动底层`spi`或`i2c`收发数据，具体实现功能如下

* 按照不同板子初始化SPI/I2C与GPIO口，并把对应的驱动绑定在SPI上
* 给驱动层提供硬件无关的SPI/I2C操作
* 使用轮询或者DMA驱动SPI/I2C收发数据

## 工作队列

同一个SPI/I2C的驱动都运行在同一个工作队列中.一个SPI/I2C占有一个工作队列.

```CPP
static constexpr wq_config_t SPI0{"wq:SPI0", 2392, -1};
static constexpr wq_config_t SPI1{"wq:SPI1", 2392, -2};
static constexpr wq_config_t SPI2{"wq:SPI2", 2392, -3};
static constexpr wq_config_t SPI3{"wq:SPI3", 2392, -4};
static constexpr wq_config_t SPI4{"wq:SPI4", 2392, -5};
static constexpr wq_config_t SPI5{"wq:SPI5", 2392, -6};
static constexpr wq_config_t SPI6{"wq:SPI6", 2392, -7};

static constexpr wq_config_t I2C0{"wq:I2C0", 2336, -8};
static constexpr wq_config_t I2C1{"wq:I2C1", 2336, -9};
static constexpr wq_config_t I2C2{"wq:I2C2", 2336, -10};
static constexpr wq_config_t I2C3{"wq:I2C3", 2336, -11};
static constexpr wq_config_t I2C4{"wq:I2C4", 2336, -12};
```

## bus

一个SPI/I2C对应一个bus.

```CPP
const wq_config_t &
device_bus_to_wq(uint32_t device_id_int)
{
 union device::Device::DeviceId device_id;
 device_id.devid = device_id_int;

 const device::Device::DeviceBusType bus_type = device_id.devid_s.bus_type;
 const uint8_t bus = device_id.devid_s.bus;

 if (bus_type == device::Device::DeviceBusType_I2C) {
  switch (bus) {
  case 0: return wq_configurations::I2C0;

  case 1: return wq_configurations::I2C1;

  case 2: return wq_configurations::I2C2;

  case 3: return wq_configurations::I2C3;

  case 4: return wq_configurations::I2C4;
  }

 } else if (bus_type == device::Device::DeviceBusType_SPI) {
  switch (bus) {
  case 0: return wq_configurations::SPI0;

  case 1: return wq_configurations::SPI1;

  case 2: return wq_configurations::SPI2;

  case 3: return wq_configurations::SPI3;

  case 4: return wq_configurations::SPI4;

  case 5: return wq_configurations::SPI5;

  case 6: return wq_configurations::SPI6;
  }
 }

 // otherwise use high priority
 return wq_configurations::hp_default;
};
```

## 例子

以`ICM42688p`的驱动为例,讲解PX4的SPI/I2C框架

### 类声明

```CPP
class ICM42688P : public device::SPI, public I2CSPIDriver<ICM42688P>
```

其中,`device::SPI`与`nuttx`的`character device`类沟通，用于具体硬件初始化等，`I2CSPIDriver<ICM42688p>`是`PX4`的`SPI`支持，用于绑定驱动到具体`SPI`.

```CPP
class __EXPORT SPI : public CDev
```

在`platforms/nuttx/Nuttx/nuttx/arch/arm/src/stm32_spi.c`中通过注册回调函数，实现了标准化SPI接口.

```CPP
static const struct spi_ops_s g_sp3iops =
{
  .lock              = spi_lock,
  .select            = stm32_spi3select,
  .setfrequency      = spi_setfrequency,
#ifdef CONFIG_SPI_DELAY_CONTROL
  .setdelay          = spi_setdelay,
#endif
  .setmode           = spi_setmode,
  .setbits           = spi_setbits,
#ifdef CONFIG_SPI_HWFEATURES
  .hwfeatures        = spi_hwfeatures,
#endif
  .status            = stm32_spi3status,
#ifdef CONFIG_SPI_CMDDATA
  .cmddata           = stm32_spi3cmddata,
#endif
  .send              = spi_send,
#ifdef CONFIG_SPI_EXCHANGE
  .exchange          = spi_exchange,
#else
  .sndblock          = spi_sndblock,
  .recvblock         = spi_recvblock,
#endif
#ifdef CONFIG_SPI_TRIGGER
  .trigger           = spi_trigger,
#endif
#ifdef CONFIG_SPI_CALLBACK
  .registercallback  = stm32_spi3register,  /* provided externally */
#else
  .registercallback  = 0,                   /* not implemented */
#endif
};
```

```CPP
template<class T>
class I2CSPIDriver : public I2CSPIDriverBase;
class I2CSPIDriverBase : public px4::ScheduledWorkItem, public I2CSPIInstance;
```

`I2CSPIDriver`实现了工作队列，绑定驱动的功能.

### 入口点

```CPP
extern "C" int icm42688p_main(int argc, char *argv[])
{
 int ch;
 using ThisDriver = ICM42688P;
 BusCLIArguments cli{false, true};
 cli.default_spi_frequency = SPI_SPEED;

 while ((ch = cli.getOpt(argc, argv, "C:R:6")) != EOF) {
  switch (ch) {
  case 'C':
   cli.custom1 = atoi(cli.optArg());
   break;

  case 'R':
   cli.rotation = (enum Rotation)atoi(cli.optArg());
   break;

  case '6':
   cli.custom2 = DRV_IMU_DEVTYPE_ICM42686P;
   break;
  }
 }

 const char *verb = cli.optArg();

 if (!verb) {
  ThisDriver::print_usage();
  return -1;
 }

 BusInstanceIterator iterator(cli.custom2 == DRV_IMU_DEVTYPE_ICM42686P ? "icm42686p" : MODULE_NAME, cli,
         cli.custom2 == DRV_IMU_DEVTYPE_ICM42686P ? DRV_IMU_DEVTYPE_ICM42686P : DRV_IMU_DEVTYPE_ICM42688P);

 if (!strcmp(verb, "start")) {
  return ThisDriver::module_start(cli, iterator);
 }

 if (!strcmp(verb, "stop")) {
  return ThisDriver::module_stop(iterator);
 }

 if (!strcmp(verb, "status")) {
  return ThisDriver::module_status(iterator);
 }

 ThisDriver::print_usage();
 return -1;
}
```

设置SPI时钟的频率为`SPI_SPEED`,定义在`icm42688p`的头文件里.

处理命令行参数输入，比如`rotation`,`bus`等参数.

```CPP
static int I2CSPIDriver::module_start(const BusCLIArguments &cli, BusInstanceIterator &iterator)
{
  return I2CSPIDriverBase::module_start(cli, iterator, &T::print_usage, InstantiateHelper<T>::m);
}
int I2CSPIDriverBase::module_start(const BusCLIArguments &cli, BusInstanceIterator &iterator,
       void(*print_usage)(), instantiate_method instantiate)
{
 if (iterator.configuredBusOption() == I2CSPIBusOption::All) {
  PX4_ERR("need to specify a bus type");
  print_usage();
  return -1;
 }

 bool started = false;

 while (iterator.next()) {
  if (iterator.instance()) {
   PX4_WARN("Already running on bus %i", iterator.bus());
   continue;
  }


  device::Device::DeviceId device_id{};
  device_id.devid_s.bus = iterator.bus();

  switch (iterator.busType()) {
#if defined(CONFIG_I2C)

  case BOARD_I2C_BUS: device_id.devid_s.bus_type = device::Device::DeviceBusType_I2C; break;
#endif // CONFIG_I2C

#if defined(CONFIG_SPI)

  case BOARD_SPI_BUS: device_id.devid_s.bus_type = device::Device::DeviceBusType_SPI; break;
#endif // CONFIG_SPI

  case BOARD_INVALID_BUS: device_id.devid_s.bus_type = device::Device::DeviceBusType_UNKNOWN; break;
  }


  const px4::wq_config_t &wq_config = px4::device_bus_to_wq(device_id.devid);
  I2CSPIDriverConfig driver_config{cli, iterator, wq_config};
  const int runtime_instance = iterator.runningInstancesCount();
  I2CSPIDriverInitializing initializer_data{driver_config, instantiate, runtime_instance};
  // initialize the object and bus on the work queue thread - this will also probe for the device
  px4::WorkItemSingleShot initializer(wq_config, initializer_trampoline, &initializer_data);
  initializer.ScheduleNow();
  initializer.wait();
  I2CSPIDriverBase *instance = initializer_data.instance;

  if (!instance) {
   PX4_DEBUG("instantiate failed (no device on bus %i (devid 0x%x)?)", iterator.bus(), iterator.devid());
   continue;
  }

#if defined(CONFIG_I2C)

  if (cli.i2c_address != 0 && instance->_i2c_address == 0) {
   PX4_ERR("Bug: driver %s does not pass the I2C address to I2CSPIDriverBase", instance->ItemName());
  }

#endif // CONFIG_I2C

  iterator.addInstance(instance);
  started = true;

  // print some info that we are running
  switch (iterator.busType()) {
#if defined(CONFIG_I2C)

  case BOARD_I2C_BUS:
   PX4_INFO_RAW("%s #%i on I2C bus %d", instance->ItemName(), runtime_instance, iterator.bus());

   if (iterator.external()) {
    PX4_INFO_RAW(" (external)");
   }

   if (cli.i2c_address != 0) {
    PX4_INFO_RAW(" address 0x%X", cli.i2c_address);
   }

   if (cli.rotation != 0) {
    PX4_INFO_RAW(" rotation %d", cli.rotation);
   }

   PX4_INFO_RAW("\n");

   break;
#endif // CONFIG_I2C
#if defined(CONFIG_SPI)

  case BOARD_SPI_BUS:
   PX4_INFO_RAW("%s #%i on SPI bus %d", instance->ItemName(), runtime_instance, iterator.bus());

   if (iterator.external()) {
    PX4_INFO_RAW(" (external, equal to '-b %i')", iterator.externalBusIndex());
   }

   if (cli.rotation != 0) {
    PX4_INFO_RAW(" rotation %d", cli.rotation);
   }

   PX4_INFO_RAW("\n");

   break;
#endif // CONFIG_SPI

  case BOARD_INVALID_BUS:
   break;
  }
 }

 if (!started && !cli.quiet_start) {
  static constexpr char no_instance_started[] {"no instance started (no device on bus?)"};

  if (iterator.external()) {
   PX4_WARN("%s: %s", px4_get_taskname(), no_instance_started);

  } else {
   PX4_ERR("%s: %s", px4_get_taskname(), no_instance_started);
  }

#if defined(CONFIG_I2C)

  if (iterator.busType() == BOARD_I2C_BUS && cli.i2c_address == 0) {
   PX4_ERR("%s: driver does not set i2c address", px4_get_taskname());
  }

#endif // CONFIG_I2C
 }

 return started ? 0 : -1;
}
```

根据板级信息，绑定驱动到对应的SPI/I2C中.

同时调用`instantiate`函数，实例化驱动，并初始化.

```CPP
static I2CSPIDriverBase *instantiate_default(const I2CSPIDriverConfig &config, int runtime_instance)
 {
  T *instance = new T(config);

  if (!instance) {
   PX4_ERR("alloc failed");
   return nullptr;
  }

  if (OK != instance->init()) {
   delete instance;
   return nullptr;
  }

  return instance;
 }
```

### 板级信息

班级信息存储在，`boards/px4/fmu-v6x/src/spi.cpp`中.

```CPP
constexpr px4_spi_bus_all_hw_t px4_spi_buses_all_hw[BOARD_NUM_SPI_CFG_HW_VERSIONS] = {
 initSPIFmumID(V6X_0, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM20649, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42688P, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin8}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
   initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin4}, SPI::DRDY{GPIO::PortI, GPIO::Pin6}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),

 initSPIFmumID(V6X_1, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM20649, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42688P, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin8}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
   initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin4}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),

 initSPIFmumID(V6X_3, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42670P, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42688P, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin8}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
   initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin4}, SPI::DRDY{GPIO::PortI, GPIO::Pin6}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),

 initSPIFmumID(V6X_4, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42670P, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42688P, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM20649, SPI::CS{GPIO::PortI, GPIO::Pin4}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),

 initSPIFmumID(V6X_6, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM45686, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_IIM42652, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_IMU_DEVTYPE_ADIS16470, SPI::CS{GPIO::PortI, GPIO::Pin4}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),

 initSPIFmumID(V6X_8, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM45686, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM45686, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM45686, SPI::CS{GPIO::PortI, GPIO::Pin4}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),


 initSPIFmumID(V6X_16, {
  initSPIBus(SPI::Bus::SPI1, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM20602, SPI::CS{GPIO::PortI, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
  }, {GPIO::PortI, GPIO::Pin11}),
  initSPIBus(SPI::Bus::SPI2, {
   initSPIDevice(DRV_IMU_DEVTYPE_ICM42688P, SPI::CS{GPIO::PortH, GPIO::Pin5}, SPI::DRDY{GPIO::PortA, GPIO::Pin10}),
  }, {GPIO::PortF, GPIO::Pin4}),
  initSPIBus(SPI::Bus::SPI3, {
   initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin8}, SPI::DRDY{GPIO::PortI, GPIO::Pin7}),
   initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortI, GPIO::Pin4}),
  }, {GPIO::PortE, GPIO::Pin7}),
  //  initSPIBus(SPI::Bus::SPI4, {
  //    // no devices
  // TODO: if enabled, remove GPIO_VDD_3V3_SENSORS4_EN from board_config.h
  //  }, {GPIO::PortG, GPIO::Pin8}),
  initSPIBus(SPI::Bus::SPI5, {
   initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortG, GPIO::Pin7})
  }),
  initSPIBusExternal(SPI::Bus::SPI6, {
   initSPIConfigExternal(SPI::CS{GPIO::PortI, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
   initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
  }),
 }),
};
```

```CPP
 initSPIBus(SPI::Bus::SPI1, {
  initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortA, GPIO::Pin3}, SPI::DRDY{GPIO::PortA, GPIO::Pin1}),
  initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortA, GPIO::Pin2}, SPI::DRDY{GPIO::PortA, GPIO::Pin0}),
 }),
```

将SPI与驱动类型，绑定CS与DateReady的GPIO口.

在`boards/fmu-v6x/nuttx-config/include/board.h`中

```CPP

#define ADJ_SLEW_RATE(p) (((p) & ~GPIO_SPEED_MASK) | (GPIO_SPEED_2MHz))

#define GPIO_SPI1_MISO   GPIO_SPI1_MISO_3               /* PG9  */
#define GPIO_SPI1_MOSI   GPIO_SPI1_MOSI_2               /* PB5  */
#define GPIO_SPI1_SCK    ADJ_SLEW_RATE(GPIO_SPI1_SCK_1) /* PA5  */

#define GPIO_SPI2_MISO   GPIO_SPI2_MISO_3               /* PI2  */
#define GPIO_SPI2_MOSI   GPIO_SPI2_MOSI_4               /* PI3  */
#define GPIO_SPI2_SCK    ADJ_SLEW_RATE(GPIO_SPI2_SCK_6) /* PI1  */

#define GPIO_SPI3_MISO   GPIO_SPI3_MISO_2               /* PC11 */
#define GPIO_SPI3_MOSI   GPIO_SPI3_MOSI_3               /* PB2  */
#define GPIO_SPI3_SCK    ADJ_SLEW_RATE(GPIO_SPI3_SCK_2) /* PC10 */

#define GPIO_SPI5_MISO   GPIO_SPI5_MISO_2               /* PH7  */
#define GPIO_SPI5_MOSI   GPIO_SPI5_MOSI_1               /* PF11 */
#define GPIO_SPI5_SCK    ADJ_SLEW_RATE(GPIO_SPI5_SCK_1) /* PF7  */

#define GPIO_SPI6_MISO   GPIO_SPI6_MISO_2               /* PA6  */
#define GPIO_SPI6_MOSI   GPIO_SPI6_MOSI_1               /* PG14 */
#define GPIO_SPI6_SCK    ADJ_SLEW_RATE(GPIO_SPI6_SCK_3) /* PB3  */
```

将GPI0口与SPI进行了绑定.

在`boards/fmu-v6x/nuttx-config/include/board_dma_map.h`中，定义了DMA通道

```CPP
#define DMAMAP_SPI2_RX    DMAMAP_DMA12_SPI2RX_0 /* DMA1:39 */
#define DMAMAP_SPI2_TX    DMAMAP_DMA12_SPI2TX_0 /* DMA1:40 */
```

在`boards/fmu-v6x/nuttx-config/nsh/defconfig`中，自动生成了要使用的SPI

```CPP
CONFIG_STM32H7_SPI1=y
CONFIG_STM32H7_SPI1_DMA=y
CONFIG_STM32H7_SPI1_DMA_BUFFER=1024
CONFIG_STM32H7_SPI2=y
CONFIG_STM32H7_SPI2_DMA=y
CONFIG_STM32H7_SPI2_DMA_BUFFER=4096
CONFIG_STM32H7_SPI3=y
CONFIG_STM32H7_SPI3_DMA=y
CONFIG_STM32H7_SPI3_DMA_BUFFER=1024
CONFIG_STM32H7_SPI5=y
CONFIG_STM32H7_SPI6=y
CONFIG_STM32H7_SPI6_DMA=y
CONFIG_STM32H7_SPI6_DMA_BUFFER=1024
```

在`platforms/nuttx/Nuttx/nuttx/arch/arm/src/stm32_spi.c`中，`nuttx`使用这些信息，来初始化设备.

```CPP
#  if defined(CONFIG_STM32H7_SPI1_DMA_BUFFER) && \
            CONFIG_STM32H7_SPI1_DMA_BUFFER > 0
#    define SPI1_DMABUFSIZE_ADJUSTED SPIDMA_SIZE(CONFIG_STM32H7_SPI1_DMA_BUFFER)
#    define SPI1_DMABUFSIZE_ALGN SPIDMA_BUF_ALIGN
#  endif
...
#if defined(SPI1_DMABUFSIZE_ADJUSTED)
static uint8_t g_spi1_txbuf[SPI1_DMABUFSIZE_ADJUSTED] SPI1_DMABUFSIZE_ALGN;
static uint8_t g_spi1_rxbuf[SPI1_DMABUFSIZE_ADJUSTED] SPI1_DMABUFSIZE_ALGN;
#endif
static struct stm32_spidev_s g_spi1dev =
{
  .spidev   =
              {
               &g_sp1iops
              },
  .spibase  = STM32_SPI1_BASE,
  .spiclock = SPI123_KERNEL_CLOCK_FREQ,
  .spiirq   = STM32_IRQ_SPI1,
#ifdef CONFIG_STM32H7_SPI1_DMA
  .rxch     = DMAMAP_SPI1_RX,
  .txch     = DMAMAP_SPI1_TX,
#  if defined(SPI1_DMABUFSIZE_ADJUSTED)
  .rxbuf    = g_spi1_rxbuf,
  .txbuf    = g_spi1_txbuf,
  .buflen   = SPI1_DMABUFSIZE_ADJUSTED,
#  endif
#endif
#ifdef CONFIG_PM
  .pm_cb.prepare = spi_pm_prepare,
#endif
#ifdef CONFIG_STM32H7_SPI1_COMMTYPE
  .config   = CONFIG_STM32H7_SPI1_COMMTYPE,
#else
  .config   = FULL_DUPLEX,
#endif
};
#endif /* CONFIG_STM32H7_SPI1 */
```

总之，如果用户需要修改硬件板级信息，需要使用`make menuconfig`修改`boards/fmu-v6x/nuttx-config/nsh/defconfig`,修改`boards/px4/fmu-v6x/src/spi.cpp`，`boards/fmu-v6x/nuttx-config/include/board.h`定义GPIO口，修改`boards/fmu-v6x/nuttx-config/include/board_dma_map.h`定义`DMA`通道.

### 实例化与初始化

```CPP
ICM42688P::ICM42688P(const I2CSPIDriverConfig &config) :
 SPI(config),
 I2CSPIDriver(config),
 _drdy_gpio(config.drdy_gpio),
 _px4_accel(get_device_id(), config.rotation),
 _px4_gyro(get_device_id(), config.rotation)
{
 isICM686 = config.custom2 == DRV_IMU_DEVTYPE_ICM42686P;

 if (config.drdy_gpio != 0) {
  _drdy_missed_perf = perf_alloc(PC_COUNT, MODULE_NAME": DRDY missed");
 }

 if (config.custom1 != 0) {
  _enable_clock_input = true;
  _input_clock_freq = config.custom1;
  ConfigureCLKIN();

 } else {
  _enable_clock_input = false;
 }

 ConfigureSampleRate(_px4_gyro.get_max_rate_hz());
}
```

还会根据最大的接受数据频率来配置`FIFO`中断水位.

```CPP
int ICM42688P::init()
{
 int ret = SPI::init();

 if (ret != PX4_OK) {
  DEVICE_DEBUG("SPI::init failed (%i)", ret);
  return ret;
 }

 return Reset() ? 0 : -1;
}
```

在`init`函数里调用了`SPI::init()`函数,开始初始化对应的SPI。

```CPP
int
SPI::init()
{
 /* attach to the spi bus */
 if (_dev == nullptr) {
  int bus = get_device_bus();

  if (!board_has_bus(BOARD_SPI_BUS, bus)) {
   return -ENOENT;
  }

  _dev = px4_spibus_initialize(bus);
 }

 if (_dev == nullptr) {
  DEVICE_DEBUG("failed to init SPI");
  return -ENOENT;
 }

 /* deselect device to ensure high to low transition of pin select */
 SPI_SELECT(_dev, _device, false);

 /* call the probe function to check whether the device is present */
 int ret = probe();

 if (ret != OK) {
  DEVICE_DEBUG("probe failed");
  return ret;
 }

 /* do base class init, which will create the device node, etc. */
 ret = CDev::init();

 if (ret != OK) {
  DEVICE_DEBUG("cdev init failed");
  return ret;
 }

 /* tell the world where we are */
 DEVICE_DEBUG("on SPI bus %d at %"  PRId32 " (%"  PRId32 " KHz)", get_device_bus(), PX4_SPI_DEV_ID(_device),
       _frequency / 1000);

 return PX4_OK;
}
```

其中，`px4_spibus_initialize`是`px4`定义的hal库函数.用于初始化SPI.

### 复位函数

```CPP
bool ICM42688P::Reset()
{
 _state = STATE::RESET;
 DataReadyInterruptDisable();
 ScheduleClear();
 ScheduleNow();
 return true;
}
```

标准的SPI没有`DataReady`的GPIO口，所以需要驱动来控制DataReady的行为.

```CPP
bool ICM42688P::DataReadyInterruptConfigure()
{
 if (_drdy_gpio == 0) {
  return false;
 }

 // Setup data ready on falling edge
 return px4_arch_gpiosetevent(_drdy_gpio, false, true, true, &DataReadyInterruptCallback, this) == 0;
}

bool ICM42688P::DataReadyInterruptDisable()
{
 if (_drdy_gpio == 0) {
  return false;
 }

 return px4_arch_gpiosetevent(_drdy_gpio, false, false, false, nullptr, nullptr) == 0;
}
```

这个会开启GPIO口中断与注册回调函数.

其余的SPI标准接口由nuttx控制。

### 收发数据

```CPP
int
SPI::transfer(uint8_t *send, uint8_t *recv, unsigned len)
{
 int result;

 if ((send == nullptr) && (recv == nullptr)) {
  return -EINVAL;
 }

 LockMode mode = up_interrupt_context() ? LOCK_NONE : _locking_mode;

 /* lock the bus as required */
 switch (mode) {
 default:
 case LOCK_PREEMPTION: {
   irqstate_t state = px4_enter_critical_section();
   result = _transfer(send, recv, len);
   px4_leave_critical_section(state);
  }
  break;

 case LOCK_THREADS:
  SPI_LOCK(_dev, true);
  result = _transfer(send, recv, len);
  SPI_LOCK(_dev, false);
  break;

 case LOCK_NONE:
  result = _transfer(send, recv, len);
  break;
 }

 return result;
}
```

这个函数收发SPI数据到指定的存储空间中.

```CPP
int
SPI::_transfer(uint8_t *send, uint8_t *recv, unsigned len)
{
 SPI_SETFREQUENCY(_dev, _frequency);
 SPI_SETMODE(_dev, _mode);
 SPI_SETBITS(_dev, 8);
 SPI_SELECT(_dev, _device, true);

 /* do the transfer */
 SPI_EXCHANGE(_dev, send, recv, len);

 /* and clean up */
 SPI_SELECT(_dev, _device, false);

 return PX4_OK;
}

```

这个函数与`nuttx`沟通，驱动SPI按照DMA或轮询的方法收发数据.如果不支持DMA或数据包小于阈值，`SPI`会以轮询的方式驱动.
