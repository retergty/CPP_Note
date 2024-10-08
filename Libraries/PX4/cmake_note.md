# CMake

本文总结`PX4`CMake文件的大致流程，便于日后梳理启动过程.

## Makefile

`PX4`通过`makefile`启动,但是按照`makefile`文件里的注释，它只不过是为了实现编译以外的复杂功能而使用的，比如启动仿真器,下载二进制文件等。单独使用`CMake`命令也可以启动编译.

对于`PX4`的`Makefile`不作研究.

## 顶层CMakeLists

顶层`CMakeLists.txt`统领所有的`PX4`代码编译流程.

通过传递或者是设置环境变量`CONFIG`,就可以决定编译的飞控板.

### px4_config.cmake

根据`CONFIG`变量的值,文件`px4_config.cmake`就进行选择飞控板的任务.

```CMake
if(NOT CONFIG)
  # default to px4_ros2_default if building within a ROS2 colcon environment
  if(("$ENV{COLCON}" MATCHES "1") AND ("$ENV{ROS_VERSION}" MATCHES "2"))
    set(CONFIG "px4_ros2_default" CACHE STRING "desired configuration")
  else()
    set(CONFIG "px4_sitl_default" CACHE STRING "desired configuration")
  endif()
endif()
```

如果未设置`CONFIG`变量，默认为`px4_sitl_default`.

```CMake
if(NOT PX4_CONFIG_FILE)

  file(GLOB_RECURSE board_configs
    RELATIVE "${PX4_SOURCE_DIR}/boards"
    "boards/*.px4board"
    )

  foreach(filename ${board_configs})
    # parse input CONFIG into components to match with existing in tree configs
    #  the platform prefix (eg nuttx_) is historical, and removed if present
    string(REPLACE ".px4board" "" filename_stripped ${filename})
    string(REPLACE "/" ";" config ${filename_stripped})
    list(LENGTH config config_len)

    if(${config_len} EQUAL 3)
      list(GET config 0 vendor)
      list(GET config 1 model)
      list(GET config 2 label)

      set(board "${vendor}${model}")

      # <VENDOR>_<MODEL>_<LABEL> (eg px4_fmu-v2_default)
      # <VENDOR>_<MODEL>_default (eg px4_fmu-v2) # allow skipping label if "default"
      if ((${CONFIG} MATCHES "${vendor}_${model}_${label}") OR # match full vendor, model, label
          ((${label} STREQUAL "default") AND (${CONFIG} STREQUAL "${vendor}_${model}")) # default label can be omitted
      )
        set(PX4_CONFIG_FILE "${PX4_SOURCE_DIR}/boards/${filename}" CACHE FILEPATH "path to PX4 CONFIG file" FORCE)
        set(PX4_BOARD_DIR "${PX4_SOURCE_DIR}/boards/${vendor}/${model}" CACHE STRING "PX4 board directory" FORCE)
        set(MODEL "${model}" CACHE STRING "PX4 board model" FORCE)
        set(VENDOR "${vendor}" CACHE STRING "PX4 board vendor" FORCE)
        set(LABEL "${label}" CACHE STRING "PX4 board vendor" FORCE)
        break()
      endif()

      # <BOARD>_<LABEL> (eg px4_fmu-v2_default)
      # <BOARD>_default (eg px4_fmu-v2) # allow skipping label if "default"
      if ((${CONFIG} MATCHES "${board}_${label}") OR # match full board, label
          ((${label} STREQUAL "default") AND (${CONFIG} STREQUAL "${board}")) # default label can be omitted
      )
        set(PX4_CONFIG_FILE "${PX4_SOURCE_DIR}/boards/${filename}" CACHE FILEPATH "path to PX4 CONFIG file" FORCE)
        set(PX4_BOARD_DIR "${PX4_SOURCE_DIR}/boards/${vendor}/${model}" CACHE STRING "PX4 board directory" FORCE)
        set(MODEL "${model}" CACHE STRING "PX4 board model" FORCE)
        set(VENDOR "${vendor}" CACHE STRING "PX4 board vendor" FORCE)
        set(LABEL "${label}" CACHE STRING "PX4 board vendor" FORCE)
        break()
      endif()
    endif()
  endforeach()
endif()
```

实际上就是根据`CONFIG`变量的值，选择`boards`里对应的配置文件。以`px4_sitl_default`为例，会设置的变量值如下

* `PX4_CONFIG_FILE`为`PX4-Autopilot/boards/px4/sitl/default.px4board`
* `PX4_BOARD_DIR`为`PX4-Autopilot/boards/px4/sitl`
* `MODEL`为`sitl`
* `VENDOR`为`px4`
* `LABEL`为`default`

```CMake
set(PX4_BOARD ${VENDOR}_${MODEL} CACHE STRING "PX4 board" FORCE)

# board name is uppercase with no underscores when used as a define
string(TOUPPER ${PX4_BOARD} PX4_BOARD_NAME)
string(REPLACE "-" "_" PX4_BOARD_NAME ${PX4_BOARD_NAME})
set(PX4_BOARD_NAME ${PX4_BOARD_NAME} CACHE STRING "PX4 board define" FORCE)

set(PX4_BOARD_VENDOR ${VENDOR} CACHE STRING "PX4 board vendor" FORCE)
set(PX4_BOARD_MODEL ${MODEL} CACHE STRING "PX4 board model" FORCE)

set(PX4_BOARD_LABEL ${LABEL} CACHE STRING "PX4 board label" FORCE)

set(PX4_CONFIG "${PX4_BOARD_VENDOR}_${PX4_BOARD_MODEL}_${PX4_BOARD_LABEL}" CACHE STRING "PX4 config" FORCE)
```

以`px4_sitl_default`为例，会设置的变量值如下

* `PX4_BOARD`为`px4_sitl`
* `PX4_BOARD_NAME`为`PX4_SITL`
* `PX4_BOARD_VENDOR`为`px4`
* `PX4_BOARD_MODEL`为`sitl`
* `PX4_BOARD_LABEL`为`default`
* `PX4_CONFIG`为`px4_sitl_default`

### *.px4board

`*.px4board`包含了对于这个飞控板所需要设置的变量与所需要编译的模块.

以`fmu-v6x`里的`default.px4board`为例.

```kconfig
CONFIG_BOARD_TOOLCHAIN="arm-none-eabi"
CONFIG_BOARD_ARCHITECTURE="cortex-m7"
CONFIG_BOARD_ETHERNET=y
CONFIG_BOARD_SERIAL_GPS1="/dev/ttyS0"
CONFIG_BOARD_SERIAL_GPS2="/dev/ttyS7"
CONFIG_BOARD_SERIAL_TEL1="/dev/ttyS6"
CONFIG_BOARD_SERIAL_TEL2="/dev/ttyS4"
CONFIG_BOARD_SERIAL_TEL3="/dev/ttyS1"
CONFIG_BOARD_SERIAL_EXT2="/dev/ttyS3"
CONFIG_DRIVERS_ADC_ADS1115=y
CONFIG_DRIVERS_ADC_BOARD_ADC=y
CONFIG_DRIVERS_BAROMETER_BMP388=y
CONFIG_DRIVERS_BAROMETER_INVENSENSE_ICP201XX=y
CONFIG_DRIVERS_BAROMETER_MS5611=y
CONFIG_DRIVERS_CAMERA_CAPTURE=y
CONFIG_DRIVERS_CAMERA_TRIGGER=y
CONFIG_DRIVERS_CDCACM_AUTOSTART=y
CONFIG_COMMON_DIFFERENTIAL_PRESSURE=y
CONFIG_COMMON_DISTANCE_SENSOR=y
CONFIG_DRIVERS_DSHOT=y
CONFIG_DRIVERS_GPIO_MCP23009=y
CONFIG_DRIVERS_GPS=y
CONFIG_DRIVERS_HEATER=y
CONFIG_DRIVERS_IMU_ANALOG_DEVICES_ADIS16470=y
CONFIG_DRIVERS_IMU_BOSCH_BMI088=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM20602=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM20649=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM20948=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM42670P=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM42688P=y
CONFIG_DRIVERS_IMU_INVENSENSE_ICM45686=y
CONFIG_DRIVERS_IMU_INVENSENSE_IIM42652=y
CONFIG_COMMON_LIGHT=y
CONFIG_COMMON_MAGNETOMETER=y
CONFIG_DRIVERS_OSD_MSP_OSD=y
CONFIG_DRIVERS_POWER_MONITOR_INA226=y
CONFIG_DRIVERS_POWER_MONITOR_INA228=y
CONFIG_DRIVERS_POWER_MONITOR_INA238=y
CONFIG_DRIVERS_POWER_MONITOR_PM_SELECTOR_AUTERION=y
CONFIG_DRIVERS_PWM_OUT=y
CONFIG_DRIVERS_PX4IO=y
CONFIG_DRIVERS_RC_INPUT=y
CONFIG_DRIVERS_SAFETY_BUTTON=y
CONFIG_DRIVERS_TONE_ALARM=y
CONFIG_DRIVERS_UAVCAN=y
CONFIG_BOARD_UAVCAN_TIMER_OVERRIDE=2
CONFIG_MODULES_AIRSPEED_SELECTOR=y
CONFIG_MODULES_BATTERY_STATUS=y
CONFIG_MODULES_CAMERA_FEEDBACK=y
CONFIG_MODULES_COMMANDER=y
CONFIG_MODULES_CONTROL_ALLOCATOR=y
CONFIG_MODULES_DATAMAN=y
CONFIG_MODULES_EKF2=y
CONFIG_MODULES_ESC_BATTERY=y
CONFIG_MODULES_EVENTS=y
CONFIG_MODULES_FLIGHT_MODE_MANAGER=y
CONFIG_MODULES_FW_ATT_CONTROL=y
CONFIG_MODULES_FW_AUTOTUNE_ATTITUDE_CONTROL=y
CONFIG_MODULES_FW_POS_CONTROL=y
CONFIG_MODULES_FW_RATE_CONTROL=y
CONFIG_MODULES_GIMBAL=y
CONFIG_MODULES_GYRO_CALIBRATION=y
CONFIG_MODULES_LAND_DETECTOR=y
CONFIG_MODULES_LANDING_TARGET_ESTIMATOR=y
CONFIG_MODULES_LOAD_MON=y
CONFIG_MODULES_LOGGER=y
CONFIG_MODULES_MAG_BIAS_ESTIMATOR=y
CONFIG_MODULES_MANUAL_CONTROL=y
CONFIG_MODULES_MAVLINK=y
CONFIG_MAVLINK_DIALECT="development"
CONFIG_MODULES_MC_ATT_CONTROL=y
CONFIG_MODULES_MC_AUTOTUNE_ATTITUDE_CONTROL=y
CONFIG_MODULES_MC_HOVER_THRUST_ESTIMATOR=y
CONFIG_MODULES_MC_POS_CONTROL=y
CONFIG_MODULES_MC_RATE_CONTROL=y
CONFIG_MODULES_NAVIGATOR=y
CONFIG_MODE_NAVIGATOR_VTOL_TAKEOFF=y
CONFIG_MODULES_RC_UPDATE=y
CONFIG_MODULES_SENSORS=y
CONFIG_MODULES_TEMPERATURE_COMPENSATION=y
CONFIG_MODULES_UXRCE_DDS_CLIENT=y
CONFIG_MODULES_VTOL_ATT_CONTROL=y
CONFIG_SYSTEMCMDS_ACTUATOR_TEST=y
CONFIG_SYSTEMCMDS_BSONDUMP=y
CONFIG_SYSTEMCMDS_DMESG=y
CONFIG_SYSTEMCMDS_GPIO=y
CONFIG_SYSTEMCMDS_HARDFAULT_LOG=y
CONFIG_SYSTEMCMDS_I2C_LAUNCHER=y
CONFIG_SYSTEMCMDS_I2CDETECT=y
CONFIG_SYSTEMCMDS_LED_CONTROL=y
CONFIG_SYSTEMCMDS_MFT=y
CONFIG_SYSTEMCMDS_MTD=y
CONFIG_SYSTEMCMDS_NETMAN=y
CONFIG_SYSTEMCMDS_NSHTERM=y
CONFIG_SYSTEMCMDS_PARAM=y
CONFIG_SYSTEMCMDS_PERF=y
CONFIG_SYSTEMCMDS_REBOOT=y
CONFIG_SYSTEMCMDS_SYSTEM_TIME=y
CONFIG_SYSTEMCMDS_TOP=y
CONFIG_SYSTEMCMDS_TOPIC_LISTENER=y
CONFIG_SYSTEMCMDS_TUNE_CONTROL=y
CONFIG_SYSTEMCMDS_UORB=y
CONFIG_SYSTEMCMDS_VER=y
CONFIG_SYSTEMCMDS_WORK_QUEUE=y
```

* `CONFIG_PLATFORM_POSIX`表示使用的平台为`POSIX`默认为`Nuttx`.
* `CONFIG_BOARD_*`设置这个飞控板的信息.
* `CONFIG_DRIVERS_*`设置飞控板要编译的`Driver`
* `CONFIG_MODULES_*`设置飞控板要编译的`Module`.
* `CONFIG_SYSTEMCMDS_*`设置飞控板要编译的`SYSTEMCMD`.

### kconfig.cmake

`kconfig.cmake`读取`PX4_CONFIG_FILE`,设置环境变量，同时生成`C++`头文件，用于代码编译.

```CMake
if(EXISTS ${BOARD_DEFCONFIG})

  # Depend on BOARD_DEFCONFIG so that we reconfigure on config change
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BOARD_DEFCONFIG})

  if(${LABEL} MATCHES "default" OR ${LABEL} MATCHES "recovery" OR ${LABEL} MATCHES "bootloader" OR ${LABEL} MATCHES "canbootloader")
    # Generate boardconfig from saved defconfig
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
      ${DEFCONFIG_PATH} ${BOARD_DEFCONFIG}
      WORKING_DIRECTORY ${PX4_SOURCE_DIR}
      OUTPUT_VARIABLE DUMMY_RESULTS
    )
  else()
    # Generate boardconfig from default.px4board and {label}.px4board
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
      ${PYTHON_EXECUTABLE} ${PX4_SOURCE_DIR}/Tools/kconfig/merge_config.py Kconfig ${BOARD_CONFIG} ${PX4_BOARD_DIR}/default.px4board ${BOARD_DEFCONFIG}
      WORKING_DIRECTORY ${PX4_SOURCE_DIR}
      OUTPUT_VARIABLE DUMMY_RESULTS
    )
  endif()

  if(${LABEL} MATCHES "allyes")
    message(AUTHOR_WARNING "allyes build: allyes is for CI coverage and not for use in production")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
      ${PYTHON_EXECUTABLE} ${PX4_SOURCE_DIR}/Tools/kconfig/allyesconfig.py
      WORKING_DIRECTORY ${PX4_SOURCE_DIR}
    )
  endif()

    # Generate header file for C/C++ preprocessor
    execute_process(
  COMMAND ${CMAKE_COMMAND} -E env ${COMMON_KCONFIG_ENV_SETTINGS}
    ${GENCONFIG_PATH} --header-path ${PX4_BINARY_DIR}/px4_boardconfig.h
  WORKING_DIRECTORY ${PX4_SOURCE_DIR}
  OUTPUT_VARIABLE DUMMY_RESULTS
  )

  # parse board config options for cmake
  file(STRINGS ${BOARD_CONFIG} ConfigContents)
  foreach(NameAndValue ${ConfigContents})
    # Strip leading spaces
    string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})

    # Find variable name
    string(REGEX MATCH "^CONFIG[^=]+" Name ${NameAndValue})

    if(Name)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})

      # remove extra quotes
      string(REPLACE "\"" "" Value ${Value})

      # Set the variable
      set(${Name} ${Value} CACHE INTERNAL "BOARD DEFCONFIG: ${Name}" FORCE)

    else()
      # Find boolean not set
      string(REGEX MATCH " (CONFIG[^ ]+) is not set" Name ${NameAndValue})

      if(${CMAKE_MATCH_1})
        set(${CMAKE_MATCH_1} "" CACHE INTERNAL "BOARD DEFCONFIG: ${CMAKE_MATCH_1}" FORCE)
      endif()
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_BOARD_" Board ${NameAndValue})

    if(Board)
      string(REPLACE "CONFIG_BOARD_" "" ConfigKey ${Name})
      if(Value)
        set(${ConfigKey} ${Value})
        message(STATUS "${ConfigKey} ${Value}")
      endif()
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_USER[^=]+" Userspace ${NameAndValue})

    if(Userspace)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})
      string(REPLACE "CONFIG_USER_" "" module ${Name})
      string(TOLOWER ${module} module)
      list(APPEND config_user_list ${module})
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_DRIVERS[^=]+" Drivers ${NameAndValue})

    if(Drivers)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})
      string(REPLACE "CONFIG_DRIVERS_" "" driver ${Name})
      string(TOLOWER ${driver} driver)

      string(REPLACE "_" "/" driver_path ${driver})

      # Pattern 1 XXX / XXX_XXX
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\1" driver_p1_folder ${driver})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\2" driver_p1_subfolder ${driver})

      # Pattern 2 XXX_XXX / XXXXXX
      string(REGEX REPLACE "(^[a-z]+_[a-z0-9]+)_([a-z0-9]+).*$" "\\1" driver_p2_folder ${driver})
      string(REGEX REPLACE "(^[a-z]+_[a-z0-9]+)_([a-z0-9]+).*$" "\\2" driver_p2_subfolder ${driver})

      # Pattern 3 XXXXXX / XXX_XXX / XXXXXX
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+)_([a-z]+[a-z0-9]+).*$" "\\1" driver_p3_folder ${driver})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+)_([a-z]+[a-z0-9]+).*$" "\\2" driver_p3_subfolder ${driver})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+)_([a-z]+[a-z0-9]+).*$" "\\3" driver_p3_subsubfolder ${driver})

      # Pattern 4 XXX_XXX / XXX_XXX_XXX
      string(REGEX REPLACE "(^[a-z]+_[a-z0-9]+)_([a-z_0-9]+).*$" "\\1" driver_p4_folder ${driver})
      string(REGEX REPLACE "(^[a-z]+_[a-z0-9]+)_([a-z_0-9]+).*$" "\\2" driver_p4_subfolder ${driver})

      # Pattern 5 XXXXXX / XXXXXX / XXX_XXX
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+[a-z0-9]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\1" driver_p5_folder ${driver})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+[a-z0-9]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\2" driver_p5_subfolder ${driver})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+[a-z0-9]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\3" driver_p5_subsubfolder ${driver})

      # Trick circumvent PX4 src naming problem with underscores and slashes
      if(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver})
        list(APPEND config_module_list drivers/${driver})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_path})
        list(APPEND config_module_list drivers/${driver_path})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_p3_folder}/${driver_p3_subfolder}/${driver_p3_subsubfolder})
        list(APPEND config_module_list drivers/${driver_p3_folder}/${driver_p3_subfolder}/${driver_p3_subsubfolder})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_p1_folder}/${driver_p1_subfolder})
        list(APPEND config_module_list drivers/${driver_p1_folder}/${driver_p1_subfolder})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_p4_folder}/${driver_p4_subfolder})
        list(APPEND config_module_list drivers/${driver_p4_folder}/${driver_p4_subfolder})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_p2_folder}/${driver_p2_subfolder})
        list(APPEND config_module_list drivers/${driver_p2_folder}/${driver_p2_subfolder})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/drivers/${driver_p5_folder}/${driver_p5_subfolder}/${driver_p5_subsubfolder})
        list(APPEND config_module_list drivers/${driver_p5_folder}/${driver_p5_subfolder}/${driver_p5_subsubfolder})
      else()
        message(FATAL_ERROR "Couldn't find path for ${driver}")
      endif()
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_MODULES[^=]+" Modules ${NameAndValue})

    if(Modules)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})
      string(REPLACE "CONFIG_MODULES_" "" module ${Name})
      string(TOLOWER ${module} module)

      string(REPLACE "_" "/" module_path ${module})

      # Pattern 1 XXX / XXX_XXX
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\1" module_p1_folder ${module})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+).*$" "\\2" module_p1_subfolder ${module})

      # Pattern 2 XXX / XXX_XXX_XXX
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+_[a-z0-9]+).*$" "\\1" module_p2_folder ${module})
      string(REGEX REPLACE "(^[a-z]+)_([a-z0-9]+_[a-z0-9]+_[a-z0-9]+).*$" "\\2" module_p2_subfolder ${module})

      # Trick circumvent PX4 src naming problem with underscores and slashes
      if(EXISTS ${PX4_SOURCE_DIR}/src/modules/${module})
        list(APPEND config_module_list modules/${module})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/modules/${module_path})
        list(APPEND config_module_list modules/${module_path})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/modules/${module_p1_folder}/${module_p1_subfolder})
        list(APPEND config_module_list modules/${module_p1_folder}/${module_p1_subfolder})
      elseif(EXISTS ${PX4_SOURCE_DIR}/src/modules/${module_p2_folder}/${module_p2_subfolder})
        list(APPEND config_module_list modules/${module_p2_folder}/${module_p2_subfolder})
      else()
        message(FATAL_ERROR "Couldn't find path for ${module}")
      endif()
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_SYSTEMCMDS[^=]+" Systemcmds ${NameAndValue})

    if(Systemcmds)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})
      string(REPLACE "CONFIG_SYSTEMCMDS_" "" systemcmd ${Name})
      string(TOLOWER ${systemcmd} systemcmd)

      list(APPEND config_module_list systemcmds/${systemcmd})
    endif()

    # Find variable name
    string(REGEX MATCH "^CONFIG_EXAMPLES[^=]+" Examples ${NameAndValue})

    if(Examples)
      # Find the value
      string(REPLACE "${Name}=" "" Value ${NameAndValue})
      string(REPLACE "CONFIG_EXAMPLES_" "" example ${Name})
      string(TOLOWER ${example} example)

      list(APPEND config_module_list examples/${example})
    endif()

  endforeach()

  if (CONFIG_BOARD_PROTECTED)
      # Put every module not in userspace also to kernel list
      foreach(modpath ${config_module_list})
    get_filename_component(module ${modpath} NAME)
    list(FIND config_user_list ${module} _index)

    if (${_index} EQUAL -1)
      list(APPEND config_kernel_list ${modpath})
    endif()
      endforeach()
  endif()

  if(PLATFORM)
    # set OS, and append specific platform module path
    set(PX4_PLATFORM ${PLATFORM} CACHE STRING "PX4 board OS" FORCE)
    list(APPEND CMAKE_MODULE_PATH ${PX4_SOURCE_DIR}/platforms/${PX4_PLATFORM}/cmake)

    # platform-specific include path
    include_directories(${PX4_SOURCE_DIR}/platforms/${PX4_PLATFORM}/src/px4/common/include)
  endif()

  if(ARCHITECTURE)
    set(CMAKE_SYSTEM_PROCESSOR ${ARCHITECTURE} CACHE INTERNAL "system processor" FORCE)
  endif()

  if(TOOLCHAIN)
    set(CMAKE_TOOLCHAIN_FILE Toolchain-${TOOLCHAIN} CACHE INTERNAL "toolchain file" FORCE)
  endif()

  set(romfs_extra_files)
  set(config_romfs_extra_dependencies)
  # additional embedded metadata
  if(NOT CONSTRAINED_FLASH AND NOT EXTERNAL_METADATA AND NOT ${PX4_BOARD_LABEL} STREQUAL "test")
    list(APPEND romfs_extra_files
      ${PX4_BINARY_DIR}/parameters.json.xz
      ${PX4_BINARY_DIR}/events/all_events.json.xz
      ${PX4_BINARY_DIR}/actuators.json.xz
      )
    list(APPEND romfs_extra_dependencies
      parameters_xml
      events_json
      actuators_json
      )
  endif()
  list(APPEND romfs_extra_files ${PX4_BINARY_DIR}/component_general.json.xz)
  list(APPEND romfs_extra_dependencies component_general_json)
  set(config_romfs_extra_files ${romfs_extra_files} CACHE INTERNAL "extra ROMFS files" FORCE)
  set(config_romfs_extra_dependencies ${romfs_extra_dependencies} CACHE INTERNAL "extra ROMFS deps" FORCE)

  if(SERIAL_PORTS)
    set(board_serial_ports ${SERIAL_PORTS} PARENT_SCOPE)
  endif()

  # Serial ports
  set(board_serial_ports)
  if(SERIAL_URT6)
    list(APPEND board_serial_ports URT6:${SERIAL_URT6})
  endif()
  if(SERIAL_GPS1)
    list(APPEND board_serial_ports GPS1:${SERIAL_GPS1})
  endif()
  if(SERIAL_GPS2)
    list(APPEND board_serial_ports GPS2:${SERIAL_GPS2})
  endif()
  if(SERIAL_GPS3)
    list(APPEND board_serial_ports GPS3:${SERIAL_GPS3})
  endif()
  if(SERIAL_GPS4)
    list(APPEND board_serial_ports GPS4:${SERIAL_GPS4})
  endif()
  if(SERIAL_GPS5)
    list(APPEND board_serial_ports GPS5:${SERIAL_GPS5})
  endif()
  if(SERIAL_TEL1)
    list(APPEND board_serial_ports TEL1:${SERIAL_TEL1})
  endif()
  if(SERIAL_TEL2)
    list(APPEND board_serial_ports TEL2:${SERIAL_TEL2})
  endif()
  if(SERIAL_TEL3)
    list(APPEND board_serial_ports TEL3:${SERIAL_TEL3})
  endif()
  if(SERIAL_TEL4)
    list(APPEND board_serial_ports TEL4:${SERIAL_TEL4})
  endif()
  if(SERIAL_TEL5)
    list(APPEND board_serial_ports TEL5:${SERIAL_TEL5})
  endif()
  if(SERIAL_RC)
    list(APPEND board_serial_ports RC:${SERIAL_RC})
  endif()
  if(SERIAL_WIFI)
    list(APPEND board_serial_ports WIFI:${SERIAL_WIFI})
  endif()
  if(SERIAL_EXT2)
    list(APPEND board_serial_ports EXT2:${SERIAL_EXT2})
  endif()

  # ROMFS
  if(ROMFSROOT)
    set(config_romfs_root ${ROMFSROOT} CACHE INTERNAL "ROMFS root" FORCE)

    if(UAVCAN_PERIPHERALS)
      set(config_uavcan_peripheral_firmware ${UAVCAN_PERIPHERALS} CACHE INTERNAL "UAVCAN peripheral firmware" FORCE)
    endif()
  endif()

  if(UAVCAN_INTERFACES)
    set(config_uavcan_num_ifaces ${UAVCAN_INTERFACES} CACHE INTERNAL "UAVCAN interfaces" FORCE)
  endif()

  if(UAVCAN_TIMER_OVERRIDE)
    set(config_uavcan_timer_override ${UAVCAN_TIMER_OVERRIDE} CACHE INTERNAL "UAVCAN TIMER OVERRIDE" FORCE)
  endif()

  # OPTIONS

  if(CONSTRAINED_FLASH)
    set(px4_constrained_flash_build "1" CACHE INTERNAL "constrained flash build" FORCE)
    add_definitions(-DCONSTRAINED_FLASH)
  endif()

  if(NO_HELP)
    add_definitions(-DCONSTRAINED_FLASH_NO_HELP="https://docs.px4.io/main/en/modules/modules_main.html")
  endif()

  if(CONSTRAINED_MEMORY)
    set(px4_constrained_memory_build "1" CACHE INTERNAL "constrained memory build" FORCE)
    add_definitions(-DCONSTRAINED_MEMORY)
  endif()

  if(TESTING)
    set(PX4_TESTING "1" CACHE INTERNAL "testing enabled" FORCE)
  endif()

  if(ETHERNET)
    set(PX4_ETHERNET "1" CACHE INTERNAL "ethernet enabled" FORCE)
  endif()

  if(CRYPTO)
    set(PX4_CRYPTO "1" CACHE INTERNAL "PX4 crypto implementation" FORCE)
    add_definitions(-DPX4_CRYPTO)
  endif()

  if(LINKER_PREFIX)
    set(PX4_BOARD_LINKER_PREFIX ${LINKER_PREFIX} CACHE STRING "PX4 board linker prefix" FORCE)
  else()
    set(PX4_BOARD_LINKER_PREFIX "" CACHE STRING "PX4 board linker prefix" FORCE)
  endif()

  if(COMPILE_DEFINITIONS)
    add_definitions( ${COMPILE_DEFINITIONS})
  endif()

  if(LINUX_TARGET)
    add_definitions( "-D__PX4_LINUX" )
  endif()

  if(LOCKSTEP)
    set(ENABLE_LOCKSTEP_SCHEDULER yes)
  endif()

  if(NOLOCKSTEP)
    set(ENABLE_LOCKSTEP_SCHEDULER no)
  endif()

  if(FULL_OPTIMIZATION)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  endif()

  include(px4_impl_os)
  px4_os_prebuild_targets(OUT prebuild_targets BOARD ${PX4_BOARD})

  # add board config directory src to build modules
  file(RELATIVE_PATH board_support_src_rel ${PX4_SOURCE_DIR}/src ${PX4_BOARD_DIR})
  list(APPEND config_module_list ${board_support_src_rel}/src)

  set(config_module_list ${config_module_list})
  set(config_kernel_list ${config_kernel_list})

endif()
```