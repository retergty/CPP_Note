# pluginlib

`pluginlib`是`ROS2`生成的插件库，它可以实现运行时加载类，卸载类的功能。

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