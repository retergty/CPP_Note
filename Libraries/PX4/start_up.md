# start up

本文大致梳理`PX4`的启动过程,`PX4`启动时会读取`/etc/rcS`文件进行初始化.

## 启动特定飞控板的代码

* `rc.board_defaults`
* `rc.board_mavlink`
* `rc.board_sensors`

三个文件启动用于特定的飞控板，在`boards`文件夹对于的飞控板文件夹里.

## 选择机型

* `rc.autostart`

这个文件用于按照参数`SYS_AUTOSTART`选择机型，由`ROMFS/px4fmu_common/init.d/airframes`文件夹里的机型自动生成.

```shell
if [ ${AIRFRAME} != none ]
then
echo "Loading airframe: /etc/init.d/airframes/${AIRFRAME}"
. /etc/init.d/airframes/${AIRFRAME}
fi
```

选择机型后，便读取`ROMFS/px4fmu_common/init.d/airframes`里对应的文件.以`ROMFS/px4fmu_common/init.d/airframes/4019_x500_v2`为例.

```shell
. ${R}etc/init.d/rc.mc_defaults
```

这个文件读取旋翼无人机的`rc.mc_defaults`注意，一起设置了参数与`shell`变量，特别是`VEHICLE_TYPE`表示无人机类型.

* `rc.vehicle_setup`

这个文件用于按照无人机类别执行`rc.*_apps`

```shell
#
# Multicopter setup.
#
if [ $VEHICLE_TYPE = mc ]
then
  # Start standard multicopter apps.
  . ${R}etc/init.d/rc.mc_apps
fi
```
