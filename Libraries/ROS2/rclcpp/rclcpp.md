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
