# rclcpp

`rclcpp`提供了`C++`API操作ROS2关键组件。

参考文档

* [rclcpp: ROS Client Library for C++](https://docs.ros.org/en/jazzy/p/rclcpp/generated/index.html)

## 使用方法

在`CMakeLists.txt`中添加

```CMake
find_package(rclcpp REQUIRED)

ament_target_dependencies(targetName rclcpp)
```

注意需要把可执行文件或库文件安装到`install`

```CMake
install(TARGETS
  targetName
  DESTINATION lib/${PROJECT_NAME})
```

在`package.xml`中添加

```xml
<depend>rclcpp</depend>
```

在源代码中添加

```CPP
#include <rclcpp/rclcpp.hpp>
```

## message

消息(Messages)是ROS2节点通过网络发送数据给另一个节点的方法，不会返回响应。比如，如果`ROS2`的一个节点读取了温度传感器，那么它就可以使用`Temperature`消息发布数据，其他节点可以订阅这个数据从而接收到`Temperature`消息。

消息使用`.msg`文件描述并定义，这些文件统一存放在`ROS`包中的`msg/`文件夹里，`.msg`文件由两个区域组成，域(`fields`)与常数(`constants`).

定义消息的方法已经在[ROS_Note](../ROS_Note.md)描述了，本节描述使用消息与自定义消息的方法。

### 使用预定义消息

`ROS2`提供了许多预定义消息，这些预定义消息也以包的形式分发。

以`std_msgs`为例。

在`CMakeLists.txt`中添加，寻找并导入包信息

```CMake
find_package(std_msgs REQUIRED)
ament_target_dependencies(talker std_msgs)
```

在`package.xml`中添加，依赖包

```xml
<depend>std_msgs</depend>
```

在代码中添加，就可以使用`std_msgs`包中定义的`string`消息。

```CPP
#include "std_msgs/msg/string.hpp"
```

使用方法是名称空间

```CPP
std_msgs::msg::String
```

### 自定义消息

自定义消息实际上就是制作自定义的包。

以`example`中的`more_interfaces`中的`AddressBook.msg`为例

创建文件夹`msg`与其中的文件`AddressBook.msg`

```msg
uint8 PHONE_TYPE_HOME=0
uint8 PHONE_TYPE_WORK=1
uint8 PHONE_TYPE_MOBILE=2

string first_name
string last_name
string phone_number
uint8 phone_type
```

在`CMakeLists.txt`中添加,寻找生成包的包,按照`msg`文件生成对应的`hpp`或`h`头文件，同时生成了和包名一样的目标，声明消息中的依赖，同时导出了包依赖`rosidl_default_runtime`使得使用这个消息的包不用`find_package(rosidl_default_runtime)`.

```CMake
find_package(rosidl_default_generators REQUIRED)
set(msg_files
  "msg/AddressBook.msg"
)
rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  DEPENDENCIES geometry_msgs 
)
ament_export_dependencies(rosidl_default_runtime)
```

在`package.xml`中添加，依赖包

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
```

把这个包添加到组中

```xml
<member_of_group>rosidl_interface_packages</member_of_group>
```

这样，就会在`install`中添加`more_interface`.自定义消息就放在`install/more_interfaces/include/more_interfaces/more_interfaces/msg`.

在代码中使用

```CPP
#include "more_interfaces/msg/address_book.hpp"
```

与名称空间

```CPP
more_interfaces::msg::AddressBook
```

在编译时会自动加上`-Iinstall/more_interfaces/include/more_interfaces`的选项。

### 在相同的包内使用自定义消息

由于在相同的包内时，当前包还没有被创建，所以使用自定义消息需要在`CMakeLists.txt`中加入

```CMake
rosidl_target_interfaces(publish_address_book
  ${PROJECT_NAME} "rosidl_typesupport_cpp")
```

假设`publish_address_book`为要使用这个消息的目标。

### 给自定义消息添加测试

以`example`中的`more_interfaces`中的`AddressBook.msg`为例

使用`publish_address_book.cpp`测试

在`CMakeLists.txt`中添加,为了避免循环依赖

```CMake
add_executable(publish_address_book src/publish_address_book.cpp)

ament_target_dependencies(publish_address_book rclcpp)
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

target_link_libraries(publish_address_book "${cpp_typesupport_target}")
```

## 导出头文件路径

当其它包使用这个包时，它可能需要获取对应的头文件。

有两个方法可以导出头文件目录，它们都是把原来在`include`子目录里的头文件导出到安装目录中的`include/${PROJECT_NAME}/`.为了防止名称冲突，`include`子目录通常是包含`/${PROJECT_NAME}`的，所以代码中可以使用`${PROJECT_NAME}/xxx.hpp`来`include`这个头文件。

使用这个头文件路径的包，需要在`CMakeLists.txt`中添加

```CMake
find_package()
ament_target_dependencies()
# 或 target_link_libraries()
```

### 使用`ament_export_include_directories`

在`CMakeLists.txt`中添加

```CMake
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
ament_export_include_directories(
  include/${PROJECT_NAME}
)
```

直接导出了头文件路径。`ament_export_include_directories`里的相对路径是相对于`install/${PROJECT_NAME}`的。

### 使用`target_include_directories`

在`CMakeLists.txt`中添加

```CMake
target_include_directories(my_library
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
```

这也会导出头文件路径，由于安装时，这个目标的相对路径相对于`${CMAKE_INSTALL_PREFIX}`，通常是`install/${PROJECT_NAME}`.

## 导出库文件

`ROS2`会把所有的包安装在`install/<packageName>`中。这也是`CMake`安装目标的路径，为了导出库的公共库文件，必须要设置目标导出头文件目录，并把这些安装到对应位置。

在`CMakeLists.txt`中添加

```CMake
target_include_directories(my_library
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")
```

这样，在构建时和安装时会导出不同的头文件目录。

在`CMakeLists.txt`中添加,安装外部使用者可见的头文件

```CMake
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
```

之后需要安装库文件目标，并自动生成`config`文件，`EXPORT`后面的名字就是使用`find_package`找到的包名，可以直接是`${PROJECT_NAME}`，注意`ament_export_targets`也要修改。

```CPP
install(
  TARGETS my_library
  EXPORT export_${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_libraries(my_library)
ament_export_dependencies(some_dependency)
```

会生成对应的`config`文件，位置相对于`CMAKE_INSTALL_PREFIX`，这是由`colcon`正确设置的。通常是`install/<packageName>/share/<packageName>/cmake`

注意，`ament_cmake`会自动生成顶层`<packageName>Config.cmake`用于发现包，并`include`所有的导出配置文件。

* `ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)`便是进行`install(EXPORT)`
* `ament_export_libraries(my_library)`导出了库，使用这个目标时需要链接的库。
* `ament_export_dependencies`导出了依赖，使用这个目标时传递的依赖。

## 组合节点

一个组合节点实际上是一个共享库。

### 修改构建信息

在`package.xml`中添加`rclcpp_components`的依赖。

```xml
<depend>rclcpp_components</depend>
```

在`CMakeLists.txt`中查找`rclcpp_components`包。

```CMake
find_package(rclcpp_components REQUIRED)
```

把`add_executable`更改为`add_library`，假设原目标为`vincent_driver`,新的组合节点目标为`vincent_driver_component`

```CMake
add_library(vincent_driver_component src/vincent_driver.cpp)
```

注意在代码其余部分更新目标名。

```CMake
rclcpp_components_register_node(
    vincent_driver_component
    PLUGIN "palomino::VincentDriver"
    EXECUTABLE vincent_driver
)
```

声明组合节点。`PLUGIN`指的是把节点设置为组件，`palomino`是我们`CPP`代码里这个类所在的名称空间,`VincentDriver`表示要注册为组件的类，必须是`Node`的子类。

```CMake
ament_export_targets(export_vincent_driver_component)
install(TARGETS vincent_driver_component
        EXPORT export_vincent_driver_component
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
```

同时按照库文件的安装方法，把`install`修改为

```CMake
ament_export_targets(export_vincent_driver_component)
install(TARGETS vincent_driver_component
        EXPORT export_vincent_driver_component
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
```

### 修改代码

原先的代码为

```CPP
namespace palomino
{
    class VincentDriver : public rclcpp::Node
    {
        // ...
    };
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<palomino::VincentDriver>());
    rclcpp::shutdown();
    return 0;
}
```

把继承于`Node`的类修改为接受`NodeOptions`参数

```CPP
VincentDriver(const rclcpp::NodeOptions & options) : Node("vincent_driver", options)
{
  // ...
}
```

同时，不再有`main`函数了,并使用`RCLCPP_COMPONENTS_REGISTER_NODE`注册组件。

```CPP
#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(palomino::VincentDriver)
```

### 使用组合节点

使用组合节点分为两种方法,第一种是命令行加载组合节点,这个方法在`ROS_Note.md`中;第二种是使用代码集成组合节点.

以`composition`包下的`talker_component`,`listener_component`,`client_component`,`server_component`组合节点为例.这些名字是在`CMakeLists.txt`中的目标名.而在`C++`中对应的类是`composition::Talker`,`composition::Listener`,`composition::Server`,`composition::Client`

```CPP
// Copyright 2016 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "composition/client_component.hpp"
#include "composition/listener_component.hpp"
#include "composition/talker_component.hpp"
#include "composition/server_component.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char * argv[])
{
  // Force flush of the stdout buffer.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Initialize any global resources needed by the middleware and the client library.
  // This will also parse command line arguments one day (as of Beta 1 they are not used).
  // You must call this before using any other part of the ROS system.
  // This should be called once per process.
  rclcpp::init(argc, argv);

  // Create an executor that will be responsible for execution of callbacks for a set of nodes.
  // With this version, all callbacks will be called from within this thread (the main one).
  rclcpp::executors::SingleThreadedExecutor exec;
  rclcpp::NodeOptions options;

  // Add some nodes to the executor which provide work for the executor during its "spin" function.
  // An example of available work is executing a subscription callback, or a timer callback.
  auto talker = std::make_shared<composition::Talker>(options);
  exec.add_node(talker);
  auto listener = std::make_shared<composition::Listener>(options);
  exec.add_node(listener);
  auto server = std::make_shared<composition::Server>(options);
  exec.add_node(server);
  auto client = std::make_shared<composition::Client>(options);
  exec.add_node(client);

  // spin will block until work comes in, execute work as it becomes available, and keep blocking.
  // It will only be interrupted by Ctrl-C.
  exec.spin();

  rclcpp::shutdown();

  return 0;
}
```

注意,实际上不是组合节点也可以这样编写代码,运行的结果也是一样的.

* `composition/listener_component.hpp`就是我们定义的组件名,`ament_cmake`会自动生成`hpp`头文件.
* `composition::Listener`是我们定义的类名.

## 插件plugin

### 创建插件

一个插件由两个部分组成，一个是基类（通常是抽象基类），一个是派生类，基类用于在`C++`中实现多态，派生类用于实际的逻辑。

#### 创建基类

```shell
ros2 pkg create --build-type ament_cmake --license Apache-2.0 --dependencies pluginlib --node-name area_node polygon_base
```

创建了名为`polygon_base`的包,添加了对`pluginlib`包的依赖。

创建基类的头文件`include/polygon_base/regular_polygon.hpp`

```CPP
#ifndef POLYGON_BASE_REGULAR_POLYGON_HPP
#define POLYGON_BASE_REGULAR_POLYGON_HPP

namespace polygon_base
{
  class RegularPolygon
  {
    public:
      virtual void initialize(double side_length) = 0;
      virtual double area() = 0;
      virtual ~RegularPolygon(){}

    protected:
      RegularPolygon(){}
  };
}  // namespace polygon_base

#endif  // POLYGON_BASE_REGULAR_POLYGON_HPP
```

创建了一个名为`RegularPolygon`的抽象基类，为了防止之后的名称冲突，最好加上以包名`polygon_base`命名的名称空间。

插件的构造函数不支持传递参数，我们需要定义`initialize`来初始化插件类。

修改`polygon_base`的`CMakeLists.txt`,添加以下代码，导出这个头文件的搜索目录。

```CMake
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
ament_export_include_directories(
  include/${PROJECT_NAME}
)
```

#### 创建派生类

```shell
ros2 pkg create --build-type ament_cmake --license Apache-2.0 --dependencies polygon_base pluginlib --library-name polygon_plugins polygon_plugins
```

创建了一个名为`polygon_plugins`的包，它是一个共享库，依赖`polygon_base`与`pluginlib`。

创建派生类的源文件`src/polygon_plugins.cpp`

```CPP
#include <polygon_base/regular_polygon.hpp>
#include <cmath>

namespace polygon_plugins
{
  class Square : public polygon_base::RegularPolygon
  {
    public:
      void initialize(double side_length) override
      {
        side_length_ = side_length;
      }

      double area() override
      {
        return side_length_ * side_length_;
      }

    protected:
      double side_length_;
  };

  class Triangle : public polygon_base::RegularPolygon
  {
    public:
      void initialize(double side_length) override
      {
        side_length_ = side_length;
      }

      double area() override
      {
        return 0.5 * side_length_ * getHeight();
      }

      double getHeight()
      {
        return sqrt((side_length_ * side_length_) - ((side_length_ / 2) * (side_length_ / 2)));
      }

    protected:
      double side_length_;
  };
}

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(polygon_plugins::Square, polygon_base::RegularPolygon)
PLUGINLIB_EXPORT_CLASS(polygon_plugins::Triangle, polygon_base::RegularPolygon)
```

最后的四行真正注册这个类为`plugin`.

`PLUGINLIB_EXPORT_CLASS`包含插件派生类的名字`polygon_plugins::Square`与基类的名字`polygon_base::RegularPolygon`.

创建`plugins.xml`，让`plugin`加载器可以发现这个插件.

```xml
<library path="polygon_plugins">
  <class type="polygon_plugins::Square" base_class_type="polygon_base::RegularPolygon">
    <description>This is a square plugin.</description>
  </class>
  <class type="polygon_plugins::Triangle" base_class_type="polygon_base::RegularPolygon">
    <description>This is a triangle plugin.</description>
  </class>
</library>
```

`library`包含了`plugin`所在的共享库库名，在这里是`polygon_plugins`.

`class`包含了`plugin`派生类的类型与基类的类型。

`description`包含了描述。

修改`CMakeLists.txt`，生成插件。

```CMake
pluginlib_export_plugin_description_file(polygon_base plugins.xml)
```

`pluginlib_export_plugin_description_file`接受两个参数，`polygon_base`就是基类所在的包，`plugins.xml`就是定义了插件的`xml`文件，相对于当前CMake路径。

注意，由于使用了`ros2 pkg create`,那么已经自动生成了把包生成共享库的`CMake`代码。

### 使用插件

使用插件的包只需要依赖插件基类所在的包即可，不需要依赖派生类所在的包。

创建`src/area_node.cpp`

```CPP
#include <pluginlib/class_loader.hpp>
#include <polygon_base/regular_polygon.hpp>

int main(int argc, char** argv)
{
  // To avoid unused parameter warnings
  (void) argc;
  (void) argv;

  pluginlib::ClassLoader<polygon_base::RegularPolygon> poly_loader("polygon_base", "polygon_base::RegularPolygon");

  try
  {
    std::shared_ptr<polygon_base::RegularPolygon> triangle = poly_loader.createSharedInstance("polygon_plugins::Triangle");
    triangle->initialize(10.0);

    std::shared_ptr<polygon_base::RegularPolygon> square = poly_loader.createSharedInstance("polygon_plugins::Square");
    square->initialize(10.0);

    printf("Triangle area: %.2f\n", triangle->area());
    printf("Square area: %.2f\n", square->area());
  }
  catch(pluginlib::PluginlibException& ex)
  {
    printf("The plugin failed to load for some reason. Error: %s\n", ex.what());
  }

  return 0;
}
```

`ClassLoader`是实现插件关键的类，它接受插件基类作为模板实参。构造函数第一个实参为插件基类所在的包`"polygon_base"`，第二个实参为插件基类名`"polygon_base::RegularPolygon"`，均为字符串。

成员函数`createSharedInstance`接收插件派生基类名，创建插件派生基类，使用共享指针接收，同时调用`initialize`函数进行初始化。

在`CMakeLists.txt`中，添加对基类包`polygon_base`与插件包`pluginlib`的依赖。
