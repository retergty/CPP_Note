# Build System

参考文档

* [The build system](https://docs.ros.org/en/jazzy/Concepts/Advanced/About-Build-System.html#meta-build-tool)

`ROS2`构建系统允许开发者按照需要使用ROS2代码。`ROS2`在很大程度上依赖于将代码划分为包(package),包含一个清单文件(`package.xml`).此清单文件包含有关该包的基本元数据，包括其对其他包的依赖关系.构建工具需要这个文件才能运行。

包的创建是使用`ament`工具，而包的构建则是使用`colcon`.官方支持使用`CMake`或`Python`创建的包。

## ament_package 包

所有`ament`包都必须在包的根目录包含一个`package.xml`文件，无论其底层构建系统如何。`package.xml`清单文件包含处理和操作包所需的信息。比如包名，包依赖等。`package.xml`文件也作为指示包所在的路径的标记文件。

`package.xml`文件的解析工具是`catkin_pkg`，而搜索工具则是由构建工具(build tool)，比如`colcon`提供。

## package.xml语法

* [catkinpackage.xml](https://wiki.ros.org/catkin/package.xml)
* [Package Manifest Format Three Specification](https://ros.org/reps/rep-0149.html#package-format-3)

一个常见的`package.xml`如下

```xml
<?xml version="1.0"?>
<?xml-model href="http://download.ros.org/schema/package_format3.xsd" schematypens="http://www.w3.org/2001/XMLSchema"?>
<package format="3">
  <name>more_interfaces</name>
  <version>0.0.0</version>
  <description>more interface</description>
  <maintainer email="337467729@qq.com">hitman</maintainer>
  <license>Apache-2.0</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <buildtool_depend>rosidl_default_generators</buildtool_depend>

  <exec_depend>rosidl_default_runtime</exec_depend>

  <member_of_group>rosidl_interface_packages</member_of_group>
  <test_depend>ament_lint_auto</test_depend>
  <test_depend>ament_lint_common</test_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

### 依赖

在`package.xml`中定义了七种依赖类型。

* **Build Dependencies**，构建依赖关系指明了构建包是所需的依赖包。当构建时需要依赖包中的文件时就会出现这种情况。比如可以是源文件`include`的头文件，链接的库文件或者是任何构建期需要的任何资源。在交叉编译场景中，构建依赖项适用于目标架构。
* **Build Export Dependencies**，构建导出依赖关系指明了根据此包构建库所需的依赖包。当把依赖包的头文件也作为该包的公共头文件时就会出现这种情况。
* **Execution Dependencies**，执行依赖关系指明了运行该包的代码时所需的依赖包。比如可执行文件运行时的共享库。
* **Test Dependencies**，测试依赖关系指明了运行单元测试时所需的依赖包。它不应该重复任何已经提到的构建依赖关系或执行依赖关系。
* **Build Tool Dependencies**，构建工具依赖关系指明了当构建包时所需的构建系统工具依赖包。比如在`ROS2`中使用的构建工具依赖是`ament_cmake`
* **Documentation Tool Dependencies**,文件工具依赖关系指明了这个包生成文件时所需的依赖包。
* **group dependencies**,组依赖关系指明了该包依赖的依赖组，依赖组是一系列的相同功能包的集合，使用组依赖关系可以方便地引入一系列依赖关系。

这七种依赖关系使用如下的标签指定的：

* `<depend>`指明了构建，导出，执行的依赖包。这也是最常用的依赖标签。
* `<build_depend>`指明了构建的依赖包。
* `<build_export_depend>`指明了构建导出依赖包。
* `<exec_depend>`指明了运行依赖包
* `<test_depend>`指明了测试依赖包
* `<buildtool_depend>`指明了构建工具依赖包
* `<doc_depend>`指明了文件依赖包
* `<group_depend>`指明了组依赖关系。

### 常见选项

* `<build_type>`表示构建类型。

## CMakeLists.txt

对于`C++`代码，`ROS2`使用CMake工具构建代码，本节讨论在ROS2中常见的CMake语法。

一个常见的`CMakeList.txt`如下。

```CMake
cmake_minimum_required(VERSION 3.8)
project(more_interfaces)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
set(msg_files
  "msg/AddressBook.msg"
)
rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
)
ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()
find_package(rclcpp REQUIRED)

add_executable(publish_address_book src/publish_address_book.cpp)

ament_target_dependencies(publish_address_book rclcpp)
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

target_link_libraries(publish_address_book "${cpp_typesupport_target}")

install(TARGETS
    publish_address_book
  DESTINATION lib/${PROJECT_NAME})


ament_package()
```

### 解释

* `project(more_interfaces)`定义包的名称，必须与`package.xml`中定义的包名相同。
* `find_package(ament_cmake REQUIRED)`使用`find_package`发现包，指示了`C++`代码的依赖，或者是特殊CMake函数。
* `ament_cmake`是`ROS2`包必须的依赖包，`rosidl_default_generators`是自定义消息`msg`必须的依赖包，`rclcpp`则是`C++`代码与ROS2核心概念的接口。ROS2通过设置环境变量`CMAKE_PREFIX_PATH`来使得CMake可以查找到包。
* `rosidl_generate_interfaces`把对应的`.msg`文件转化为`.hpp`文件，存储到对应的`build`文件夹中。
* `ament_export_dependencies(rosidl_default_runtime)`命令导出依赖，使得随后依赖这个包的用户也必须依赖`rosidl_default_runtime`，这个是自定义消息的运行时支持库。
* `add_executable(publish_address_book src/publish_address_book.cpp)`表示生成一个目标`publish_address_book`，用来测试自定义消息。
* `ament_target_dependencies(publish_address_book rclcpp)`指示目标依赖的包，也就是编译生成目标时需要链接的库。
* `rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} rosidl_typesupport_cpp)`命令取得自定义消息自动生成的目标。
* `target_link_libraries(publish_address_book "${cpp_typesupport_target}")`链接自定义消息的目标，由于这些自定义消息目标并不会`ament`包，所以只能使用`target_link_libraries`.
* `install(TARGETS publish_address_book DESTINATION lib/${PROJECT_NAME})`把目标安装到指定位置，通常使用`colcon`构建后，会安装到`install/more_interfaces/lib/more_interfaces/`下。

直接使用CMake也是可以成功构建代码的，只不过是`build`文件夹与使用`colcon`的不同。在`VSCODE`中可以直接配置CMake。

## colcon

参考文档

* [colcon - collective construction](https://colcon.readthedocs.io/en/released/)

`colcon`是一个命令行工具，通常用于自动化地构建并测试多个软件包的情况。它使流程自动化、处理顺序并设置使用包的环境。

### 工作区结构

`colcon`默认的工作区结构如下

```tree
.
├── build
├── install
├── log
└── src
```

* `src`文件夹存放工作区的软件包。
* `build`文件夹存放`colcon`生成的中间文件。
* `install`文件夹存放`colcon`安装的软件以及为了使用它们而自动生成的`setup.bash`.
* `log`文件夹存放`colcon`构建时的日志文件。

### 使用多个工作区

`colcon`支持多个工作区堆叠使用，`ROS2`利用了这一特点。

```shell
source foo_ws/install/setup.bash
source bar_ws/install/setup.bash
```

上面堆叠了两个工作区。`foo_ws`叫做底层工作区`underlay`,`bar_ws`叫做顶层工作区`overlay`.

独立工作区(independent workspaces)指的是两个工作区内的包没有依赖另一个工作区的包。独立工作区可以以任意顺序堆叠。

连接工作区(Chaining workspaces)表示一个工作区依赖于另一个工作区，为了使得`colcon`可以发现依赖的包，在编译工作区时应该`source`依赖的工作区。

```shell
# Build ping_ws
cd ping_ws
colcon build
# In a new terminal source ping_ws and build pong_ws
source ping_ws/install/setup.bash
cd pong_ws
colcon build
```

可以说，顶层工作区通过提供新的包***拓展了***底层工作区。此外，如果顶层工作区同名的包还会**覆盖**底层工作区同名的包。

`colcon`目前无法直接定位底层工作区的包，只是可以使用底层工作区的`install`的包。它只会工作在当前工作区，分析当前工作区的包，默认底层工作区的依赖包都已准备完毕。

### 常用命令

`colcon`通用格式如下

```shell
colcon [verb] [--args]
```

#### build

```shell
colcon build [--args]
```

构建当前的工作区。

常用选项如下。

* `--symlink-install`尽可能使用符号链接，而不是从源目录和构建目录中复制文件。
* `--cmake-args [* [* …]]`给CMake传递参数。与其他选项匹配的参数必须以空格为前缀，例如`--cmake-args " --help"`.

#### list

```shell
colcon list
```

显示软件包，显示软件包名，在工作区的位置，包类型。

#### info

```shell
colcon info [PKG_NAME]
```

显示包信息，包括包名字，包类型，包依赖，维护者，版本号，所在位置等。

#### 选择包的选项

这些选项可以用在选择当前工作区特定的软件包，比如只构建特定的软件包。

* `--packages-up-to [PKG_NAME [PKG_NAME …]]`选择指定的包与这个包的依赖。
* `--packages-up-to-regex`选择指定的匹配正则表达式的包与这个包的依赖。
* `--packages-above [PKG_NAME [PKG_NAME …]]`选择指定的包与所有依赖于这个包的包。
* `--packages-above-and-dependencies [PKG_NAME [PKG_NAME …]]`选择指定的包和依赖或依赖于这个包的包。
* `--packages-select [PKG_NAME [PKG_NAME …]]`选择指定的包
* `--packages-select-regex [PATTERN [PATTERN …]]`选择指定的匹配正则表达式的包。

## colcon CMake package.xml关系

参考文档

* [Dependency management in ROS2: CMakeLists.txt, package.xml, colcon build, make ...?](https://answers.ros.org/question/360396/dependency-management-in-ros2-cmakeliststxt-packagexml-colcon-build-make/)

`package.xml`是为了自动打包，解决包间依赖关系而引入的文件。

在`CMake`中我们需要发现依赖包，如果依赖包在`package.xml`中使用的包名正好与CMake中的对应，那么就可以使用`ament_auto_find_build_dependencies`自动发现依赖包，但是通常会有不同，这样便需要我们手动重新声明依赖包。

由于软件包可以直接使用`CMake`构建，只需要构建前`source`底层工作区，这意味着在CMake中发现依赖包后，便已经足以构建这个软件包了。CMake发现的软件包大致分为两种，一种是提供了方便的CMake函数，另一种是链接的库文件。

但是，随着软件包的数量变多，软件包之间的依赖变得复杂，便需要一个自动化构建软件包，读取`package.xml`自动解决依赖的工具，这个便是`colcon`.

对于`colcon`的包会自动生成`<packageName>Config.cmake`用于发现包。如果使用`colcon`构建，会自动寻找到`install`安装的头文件，之后如果使用`CMake`构建，也会自动寻找到这个`<packageName>Config.cmake`从而自动寻找到`install`安装的头文件。
