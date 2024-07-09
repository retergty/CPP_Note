# command line tools

`ROS2`包含一系列的命令行工具用于探测ROS2系统的运行。所有的命令行工具都依赖于正确地设置了环境变量，也就是`source install/setup.bash`.

## 格式

```shell
ros2 sub-commands
```

`sub-commands`是子命令，每个子命令又可以接受子命令定义的参数。

## 常用子命令

### action

探测或于`ROS2`动作交互。

### bag

记录或重放`rosbag`

### component

管理组件容器.

### daemon

探测或配置`ROS2`守护进程。

### doctor

检查`ROS2`设置是否存在潜在问题。

### interface

显示有关`ROS2`接口的信息.

### launch

运行或探测启动文件。

### lifecycle

通过托管生命周期探测或管理节点

### multicast

组播调试命令

### node

探测`ROS`节点。

### param

探测或配置节点参数

### pkg

探测`ROS2`包。

#### `ros2 pkg prefix`

```shell
ros2 pkg prefix [-h] [--share] package_name
```

显示`package_name`包的安装路径

#### `ros2 pkg list`

显示所有可用的包

#### `ros2 pkg xml`

```shell
ros2 pkg xml [-h] [-t TAG] package_name
```

显示指定包的`package.xml`.

#### `ros2 pkg creat`

```shell
ros2 pkg create [-h] [--package-format {2,3}] [--description DESCRIPTION]
                       [--license LICENSE] [--destination-directory DESTINATION_DIRECTORY]
                       [--build-type {cmake,ament_cmake,ament_python}]
                       [--dependencies DEPENDENCIES [DEPENDENCIES ...]]
                       [--maintainer-email MAINTAINER_EMAIL]
                       [--maintainer-name MAINTAINER_NAME] [--node-name NODE_NAME]
                       [--library-name LIBRARY_NAME]
                       package_name

```

创建一个新的`ROS2`包，自动在当前文件目录生成一个`package_name`的文件夹，并在其中生成基础的文件比如`CMakeLists.txt`,`package.xml`.

* `package_name`是包名。
* `--package-format {2,3}`是`package.xml`中的xml版本号。
* `--description DESCRIPTION`是`packag.xml`中的`<description>`域
* `--license LICENSE`是`packag.xml`中的`<license>`域
* `--destination-directory`修改生成包的文件目录,默认是当前文件目录。
* `--build-type {cmake,ament_cmake,ament_python}`指明构建工具的类型，`cmake`则是一个单纯的cmake工程，`CMakeLists.txt`与`package.xml`中不会生成依赖于任何其它的构建工具。`ament_cmake`是CMake工程的拓展版本，在``CMakeLists.txt`中会自动添加`find_package(ament_cmake REQUIRED)`等。在`package.xml`中会自动添加`<buildtool_depend>ament_cmake</buildtool_depend>`,`<test_depend>ament_lint_auto</test_depend>`,`<test_depend>ament_lint_common</test_depend>`.
* `--dependencies DEPENDENCIES [DEPENDENCIES ...]`声明包依赖，就是在`CMakeLists.txt`中添加对应的`find_package`。在`package.xml`中添加对应的`<depend>`域，也可以
* `--maintainer-name MAINTAINER_NAME`是`packag.xml`中的`<maintainer email="hitman@todo.todo">hitman</maintainer>`
* `--maintainer-email MAINTAINER_EMAIL`是`packag.xml`中的`<maintainer email="hitman@todo.todo">hitman</maintainer>`
* `--node-name NODE_NAME`指明一个可执行文件名，会自动创建在`package_name/src`内。
* `--library-name LIBRARY_NAME`指明一个库文件名，会自动创建在`package_name/src`内。

### run

运行`ROS2`节点。

#### `ros2 run`

```shell
ros2 run [-h] [--prefix PREFIX] package_name executable_name ...
```

运行指定包中的指定可执行文件。

* `package_name`是包名
* `executable_name`是包中特定可执行文件的名字，也就是节点名。
* `argv`传递给可执行文件的参数
* `--prefix PREFIX`运行文件的前缀，会在可执行文件名前添加`PREFIX`,比如想要调试

### security

配置安全设置

### service

配置或调用ROS2服务

### test

运行ROS2启动测试

### topic

探测或发布ROS2主题

### trace

用于获取 ROS 节点执行信息的跟踪工具（仅在 Linux 上可用）。

### wtf

`doctor`的别名。
