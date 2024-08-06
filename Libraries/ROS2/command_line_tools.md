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

#### `ros2 component list`

```shell
ros2 component list [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon] [--containers-only] [container_node_name]
```

显示现在正在运行的容器与组件,以及它们的`UID`.

* `--containers-only`只显示容器

* `container_node_name`组件容器的节点名

#### `ros2 component load`

```shell
ros2 component load [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon]
                           [-n NODE_NAME] [--node-namespace NODE_NAMESPACE]
                           [--log-level LOG_LEVEL] [-r REMAP_RULES]
                           [-p PARAMETERS] [-e EXTRA_ARGUMENTS] [-q]
                           container_node_name package_name plugin_name
```

加载一个组件

* `container_node_name`组件容器的节点名
* `package_name`包名
* `plugin_name`包内的插件名,通常是一个继承了`Node`的类.

#### `ros2 component unload`

```shell
ros2 component unload [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon]
                             [-q]
                             container_node_name component_uid
                             [component_uid ...]
```

取消加载一个组件

* `container_node_name`组件容器的节点名.
* `component_uid`容器的UID.

### daemon

探测或配置`ROS2`守护进程。

### doctor

检查`ROS2`设置是否存在潜在问题。

### interface

显示有关`ROS2`接口的信息.

### launch

#### `ros2 launch`

```shell
ros2 launch [-h] [-n] [-d] [-p | -s] [-a]
                   [--launch-prefix LAUNCH_PREFIX]
                   [--launch-prefix-filter LAUNCH_PREFIX_FILTER]
                   package_name [launch_file_name] [launch_arguments ...]
```

运行或探测启动文件。

* `package_name`包含启动文件的包名
* `launch_file_name`启动文件名
* `launch_arguments`传递给启动文件的参数，格式为`<name>:=<value>`重复的参数以最后一个为准。
* `-s`,`--show-args`,`--show-arguments`显示启动文件的参数。

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

#### `ros2 pkg create`

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
* `--prefix PREFIX`运行文件的前缀，会在可执行文件名前添加`PREFIX`,比如想要调试时`gdb -ex run --args`.

### security

配置安全设置

### service

配置或调用ROS2服务

#### `ros2 service call`

```shell
ros2 service call [-h] [-r N] service_name service_type [values]
```

调用`ROS2`服务

* `service_name`服务名，包含名称空间,比如`/add_two_ints`.
* `service_type`服务类型，比如`std_srvs/srv/Empty`
* `values`服务的请求中包含的值，以`YAML`格式。
* `-r N, --rate N`以`N Hz`重复这个命令

#### `ros2 service find`

```shell
ros2 service find [-h] [-c] [--include-hidden-services] service_type
```

寻找指定类型的`ROS2`服务

* `service_type`服务的类型,比如`rcl_interfaces/srv/ListParameters`
* `-c, --count-services`只显示发现服务的数量
* `--include-hidden-services`考虑隐藏的服务

#### `ros2 service list`

```shell
ros2 service list [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon] [-t]
                         [-c] [--include-hidden-services]
```

输出所有可行的服务

* `-t, --show-types`同时显示服务类型
* `-c, --count-services`只显示发现的服务数量
* `--include-hidden-services`考虑隐藏的服务

### test

运行ROS2启动测试

### topic

探测或发布ROS2主题

#### `ros2 topic info`

```shell
ros2 topic info [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon]
                       [--verbose]
                       topic_name

```

打印特定主题的信息

* `topic_name`是主题名，包含名称空间，比如`/chatter`.
* `--verbose, -v`打印具体的信息，比如节点名，节点名称空间，主题类型，连接到主题的发布者与接收者的QoS配置。

#### `ros2 topic list`

```shell
ros2 topic list [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon]
                       [-t] [-c] [--include-hidden-topics] [-v]

```

列出可用的主题

* `-c, --count-topics`只显示主题的个数
* `--include-hidden-topics`考虑隐藏的主题
* `-v, --verbose`列出具体信息

#### `ros2 topic echo`

```shell
ros2 topic echo [-h] [--spin-time SPIN_TIME] [-s] [--no-daemon]
                       [--qos-profile {unknown,system_default,sensor_data,services_default,parameters,parameter_events,action_status_default}]
                       [--qos-depth N]
                       [--qos-history {system_default,keep_last,keep_all,unknown}]
                       [--qos-reliability {system_default,reliable,best_effort,unknown}]
                       [--qos-durability {system_default,transient_local,volatile,unknown}]
                       [--csv] [--field FIELD] [--full-length]
                       [--truncate-length TRUNCATE_LENGTH] [--no-arr]
                       [--no-str] [--flow-style] [--lost-messages]
                       [--no-lost-messages] [--raw] [--filter FILTER_EXPR]
                       [--once]
                       topic_name [message_type]
```

输出主题里的消息

* `topic_name`是主题名，包含名称空间
* `message_type`是`ROS`消息类型，比如`std_msgs/msg/String

### trace

用于获取 ROS 节点执行信息的跟踪工具（仅在 Linux 上可用）。

### wtf

`doctor`的别名。
