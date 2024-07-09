# rclcpp

`rclcpp`提供了`C++`API操作ROS2关键组件。

参考文档

* [rclcpp: ROS Client Library for C++](https://docs.ros.org/en/jazzy/p/rclcpp/generated/index.html)

## 使用方法

在`CMakeLists.txt`中添加

```CMake
find_package(rclcpp REQUIRED)
```

在`package.xml`中添加

```xml
<depend>rclcpp</depend>
```

在源代码中添加

```CPP
#include <rclcpp/rclcpp.hpp>
```

## Node

