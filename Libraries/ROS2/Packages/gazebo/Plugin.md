# plugin

插件`plugin`是一段编译为**共享库**的代码，并被插入到仿真环境中，`plugin`使得我们可以控制仿真的许多方面，比如世界，模型等。

有许多种类的插件，分别实现不同的功能。

## 传感器

参考文档

* [Sensors](https://gazebosim.org/docs/harmonic/sensors/)
* [目前所有的传感器](https://github.com/gazebosim/gz-sensors)
* [udf 传感器定义](http://sdformat.org/spec?elem=sensor&ver=1.10)

### 前置条件

在添加传感器前，需要添加几个默认插件，以使用`gazebo gui`.

```xml
<sdf version='1.9'>
  <world name='demo'>
    <plugin
        filename="gz-sim-physics-system"
        name="gz::sim::systems::Physics">
    </plugin>

    <plugin
        filename="gz-sim-scene-broadcaster-system"
        name="gz::sim::systems::SceneBroadcaster">
    </plugin>
    
    <plugin
        filename="gz-sim-user-commands-system"
        name="gz::sim::systems::UserCommands">
    </plugin>
    <!-- ... -->
```

### IMU

在`<world>`添加以下代码

```xml
<plugin filename="gz-sim-imu-system"
        name="gz::sim::systems::Imu">
</plugin>
```

这个代码定义了`IMU`传感器插件，便可以添加以下代码使用`IMU`传感器插件

```xml
<sensor name="imu_sensor" type="imu">
    <always_on>1</always_on>
    <update_rate>1</update_rate>
    <visualize>true</visualize>
    <topic>imu</topic>
</sensor>
```

* `always_on`如果为真，传感器将始终根据更新率进行更新。
* `update_rate`更新速率`Hz`
* `visualize`如果为真，传感器在`GUI`内可见
* `topic`数据发布的主题名

### 接触传感器

在`<world>`添加以下代码

```xml
<plugin filename="gz-sim-contact-system"
        name="gz::sim::systems::Contact">
</plugin>
```

这个代码定义了接触传感器插件，便可以添加以下代码使用接触传感器。

```xml
<sensor name='sensor_contact' type='contact'>
    <contact>
        <collision>collision</collision>
    </contact>
</sensor>
```

* `<collision>`标签指明要检测的碰撞名。

我们还需要添加

```xml
<plugin filename="gz-sim-touchplugin-system"
        name="gz::sim::systems::TouchPlugin">
    <target>vehicle_blue</target>
    <namespace>wall</namespace>
    <time>0.001</time>
    <enabled>true</enabled>
</plugin>
```

会在碰到墙时发布`TouchPlugin`主题。

* `target`表示当`vehicle_blue`碰到我们的传感器时，发布主题。
* `namespace`主题前面的前缀`/wall/TouchPlugin`

### 激光雷达

在`<world>`添加以下代码

```xml
<plugin
  filename="gz-sim-sensors-system"
  name="gz::sim::systems::Sensors">
  <render_engine>ogre2</render_engine>
</plugin>
```

我们就可以在对应的`<link>`下加入

```xml
<sensor name='gpu_lidar' type='gpu_lidar'>"
    <pose relative_to='lidar_frame'>0 0 0 0 0 0</pose>
    <topic>lidar</topic>
    <update_rate>10</update_rate>
    <ray>
        <scan>
            <horizontal>
                <samples>640</samples>
                <resolution>1</resolution>
                <min_angle>-1.396263</min_angle>
                <max_angle>1.396263</max_angle>
            </horizontal>
            <vertical>
                <samples>1</samples>
                <resolution>0.01</resolution>
                <min_angle>0</min_angle>
                <max_angle>0</max_angle>
            </vertical>
        </scan>
        <range>
            <min>0.08</min>
            <max>10.0</max>
            <resolution>0.01</resolution>
        </range>
    </ray>
    <always_on>1</always_on>
    <visualize>true</visualize>
</sensor>
```

* `<topic>`定义了激光雷达发布的主题
* `<update_rate>`表示更新频率`Hz`
* `<horizontal>`和`<vertical>`定义了激光雷达属性
* `<samples>`是模拟激光雷达生成的数量
* `<resolution>`该数字乘以样本以确定数据点的数量范围。
* `<min_angle>`,`<max_angle>`是生成光线的角度范围。
* `<range>`定义激光雷达最小最大的距离，以及线性精度
