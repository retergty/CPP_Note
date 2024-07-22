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

## 导出库文件头文件

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
ament_export_dependencies(some_dependency)
```

会生成对应的`config`文件，位置相对于`CMAKE_INSTALL_PREFIX`，这是由`colcon`正确设置的。通常是`install/<packageName>/share/<packageName>/cmake`

注意，对于`colcon`的包会自动生成`<packageName>Config.cmake`用于发现包。
