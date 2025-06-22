# plugin

插件`plugin`是一段编译为**共享库**的代码，并被插入到仿真环境中，`plugin`使得我们可以控制仿真的许多方面，比如世界，模型等。

有许多种类的插件，分别实现不同的功能。

插件位于`/usr/lib/x86_64-linux-gnu/gz-sim-8/plugins`中.

## 传感器

参考文档

* [Sensors](https://gazebosim.org/docs/harmonic/sensors/)
* [目前所有的传感器](https://github.com/gazebosim/gz-sim/tree/gz-sim9)
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

## 例子

### 源代码

本文以`gz-sim/examples/system_plugin`为例，讲解`gazebo`插件的写法.

```CPP
#include <gz/sim/System.hh>
namespace sample_system
{
    class SampleSystem2:
    // This class is a system.
    public gz::sim::System,
    // This class also implements the ISystemPreUpdate, ISystemUpdate,
    // and ISystemPostUpdate interfaces.
    public gz::sim::ISystemPreUpdate,
    public gz::sim::ISystemUpdate,
    public gz::sim::ISystemPostUpdate,
    public gz::sim::ISystemReset
    {
    public: SampleSystem2();

    public: ~SampleSystem2() override;

    public: void PreUpdate(const gz::sim::UpdateInfo &_info,
                gz::sim::EntityComponentManager &_ecm) override;

    public: void Update(const gz::sim::UpdateInfo &_info,
                gz::sim::EntityComponentManager &_ecm) override;

    public: void PostUpdate(const gz::sim::UpdateInfo &_info,
                const gz::sim::EntityComponentManager &_ecm) override;

    public: void Reset(const gz::sim::UpdateInfo &_info,
                    gz::sim::EntityComponentManager &_ecm) override;
    };
}
```

* 继承了`gz::sim::System`用以进行多态
* 继承了`gz::sim::ISystemPreUpdate`获得了`PreUpdate`函数，会在每次状态更新前调用.

```CPP
//! [registerSampleSystem2]
#include <gz/plugin/RegisterMore.hh>

GZ_ADD_PLUGIN(
    sample_system::SampleSystem2,
    gz::sim::System,
    sample_system::SampleSystem2::ISystemPreUpdate,
    sample_system::SampleSystem2::ISystemUpdate,
    sample_system::SampleSystem2::ISystemPostUpdate,
    sample_system::SampleSystem2::ISystemReset)
```

* 使用`GZ_ADD_PLUGIN`注册组件以及它的函数.

### CMake

```CMake
cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

find_package(gz-cmake4 REQUIRED)

project(SampleSystem)

find_package(gz-plugin3 REQUIRED COMPONENTS register)
set(GZ_PLUGIN_VER ${gz-plugin3_VERSION_MAJOR})

find_package(gz-sim9 REQUIRED)
add_library(SampleSystem SHARED SampleSystem.cc SampleSystem2.cc)
set_property(TARGET SampleSystem PROPERTY CXX_STANDARD 17)
target_link_libraries(SampleSystem
  PRIVATE gz-plugin${GZ_PLUGIN_VER}::gz-plugin${GZ_PLUGIN_VER}
  PRIVATE gz-sim9::gz-sim9)
```

### sdf

```xml
<?xml version="1.0" ?>
<sdf version="1.6">
  <world name="default">
    <plugin filename="SampleSystem"
            name="sample_system::SampleSystem2">
    </plugin>
  </world>
</sdf>
```

* 在`sdf`文件中需要指定文件名与文件中的类名，包含名称空间.

## System类

```CPP
class System
{
    /// \brief Signed integer type used for specifying priority of the
    /// execution order of PreUpdate and Update phases.
    public: using PriorityType = int32_t;

    /// \brief Default priority value for execution order of the PreUpdate
    /// and Update phases.
    public: constexpr static PriorityType kDefaultPriority = {0};

    /// \brief Name of the XML element from which the priority value will be
    /// parsed.
    public: constexpr static std::string_view kPriorityElementName =
        {"gz:system_priority"};

    /// \brief Constructor
    public: System() = default;

    /// \brief Destructor
    public: virtual ~System() = default;
};
```

`System`类是所有`plugin`的公共基类，实现了运行时多态.

在系统的仿真`update`中，有以下三个阶段

* `PreUpdate`阶段，在这个阶段中，可以读写组件。可以用于在仿真物理引擎调用前，修改模型的状态，比如应用控制信号，添加推力等.
* `Update`阶段，在这个阶段中，可以读写组件。可以用于物理仿真步中.
* `PostUpdate`阶段，在这个阶段中，只可以读取组件。在仿真物理引擎调用后，更新传感器或者是控制器。

`PreUpdate`与`Update`阶段是串行的。执行顺序通过`PriorityType`控制，数值越小，越早执行。相同`PriorityType`的`plugin`则是按照加载顺序执行。`PostUpdate`是并行的.

### ISystemConfigure类

```CPP
class ISystemConfigure {
    /// \brief Configure the system
    /// \param[in] _entity The entity this plugin is attached to.
    /// \param[in] _sdf The SDF Element associated with this system plugin.
    /// \param[in] _ecm The EntityComponentManager of the given simulation
    /// instance.
    /// \param[in] _eventMgr The EventManager of the given simulation
    /// instance.
    public: virtual void Configure(
                const Entity &_entity,
                const std::shared_ptr<const sdf::Element> &_sdf,
                EntityComponentManager &_ecm,
                EventManager &_eventMgr) = 0;
};
```

配置函数，会在系统实例化完成且所有的实体与组件从`sdf`中加载后,仿真实际开始前调用.

* `_entity`表示`plugin`连接到的实体。
* `_sdf`包含了这个实体对应的那一部分的`sdf`
* `_ecm`包含了实体所有的组件

### ISystemPreUpdate类,ISystemUpdate类

```CPP
/// \class ISystemPreUpdate ISystem.hh gz/sim/System.hh
/// \brief Interface for a system that uses the PreUpdate phase
class ISystemPreUpdate {
    public: virtual void PreUpdate(const UpdateInfo &_info,
                                    EntityComponentManager &_ecm) = 0;
};
```

```CPP
/// \class ISystemUpdate ISystem.hh gz/sim/System.hh
/// \brief Interface for a system that uses the Update phase
class ISystemUpdate {
    public: virtual void Update(const UpdateInfo &_info,
                                EntityComponentManager &_ecm) = 0;
};
```

```CPP
/// \class ISystemPostUpdate ISystem.hh gz/sim/System.hh
/// \brief Interface for a system that uses the PostUpdate phase
class ISystemPostUpdate{
    public: virtual void PostUpdate(const UpdateInfo &_info,
                                    const EntityComponentManager &_ecm) = 0;
};
```

* `_info`包含了更新时刻的信息，比如当前时间，`UpdateInfo::simTime`表示仿真引擎运行的时间点，也就是当`PreUpdate`与`Update`运行完毕后会到达的时间.

## UpdateInfo类

```CPP
/// \brief Information passed to systems on the update callback.
/// \todo(louise) Update descriptions once reset is supported.
struct UpdateInfo
{
    /// \brief Total time elapsed in simulation. This will not increase while
    /// paused.
    std::chrono::steady_clock::duration simTime{0};

    /// \brief Total wall clock time elapsed while simulation is running. This
    /// will not increase while paused.
    std::chrono::steady_clock::duration realTime{0};

    /// \brief Simulation time handled during a single update.
    std::chrono::steady_clock::duration dt{0};

    /// \brief Total number of elapsed simulation iterations.
    // cppcheck-suppress unusedStructMember
    uint64_t iterations{0};

    /// \brief True if simulation is paused, which means the simulation
    /// time is not currently running, but systems are still being updated.
    /// It is the responsibilty of a system update appropriately based on
    /// the status of paused. For example, a physics systems should not
    /// update state when paused is true.
    // cppcheck-suppress unusedStructMember
    bool paused{true};
};
```

当`*Update`函数调用时会传递给它的结构体.

* `simTime`表示当前仿真时间,也就是当`PreUpdate`与`Update`运行完毕后会到达的时间.
* `realTime`表示当前实际时间
* `dt`表示上一次更新与这一次之间的时间间隔.
* `iterations`表示已经经过了的仿真循环.
* `paused`表示仿真当前是否暂停，如果暂停，`simTime`与`realTime`不会变化.实际的物理系统不应该更新状态.

## EntityComponentManager类

```CPP
class GZ_SIM_VISIBLE EntityComponentManager;
```

`EntityComponentManager`类构建，删除或者返回实体或者组件.

这是最重要的类。
