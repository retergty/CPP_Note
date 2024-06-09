# installation

本文演示如何从源码构建`ROS2`.

参考文档

* [Ubuntu (source)](https://docs.ros.org/en/jazzy/Installation/Alternatives/Ubuntu-Development-Setup.html)

## 系统设置

### 设置`locale`

检查并设置系统`locale`为`UTF-8`.

```shell
locale  # check for UTF-8

sudo apt update && sudo apt install locales
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

locale  # verify settings
```

### 使能所需的存储库

首先确保`Ubuntu Universe`存储库已启用。

```shell
sudo apt install software-properties-common
sudo add-apt-repository universe
```

然后使用`apt`添加`ROS 2 GPG`密钥

```shell
sudo apt update && sudo apt install curl -y
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
```

然后将存储库添加到源列表中

```shell
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
```

### 安装依赖工具

```shell
sudo apt update && sudo apt install -y \
  python3-flake8-blind-except \
  python3-flake8-class-newline \
  python3-flake8-deprecated \
  python3-mypy \
  python3-pip \
  python3-pytest \
  python3-pytest-cov \
  python3-pytest-mock \
  python3-pytest-repeat \
  python3-pytest-rerunfailures \
  python3-pytest-runner \
  python3-pytest-timeout \
  ros-dev-tools
```

## 构建ROS2

### 获取ROS2源码

创建一个文件夹，存储`ROS2`源码

```shell
mkdir -p ~/ros2_jazzy/src
cd ~/ros2_jazzy
vcs import --input https://raw.githubusercontent.com/ros2/ros2/jazzy/ros2.repos src
```

### 使用`rosdep`安装依赖项

最好保证系统已经是最新的了

```shell
sudo apt upgrade
```

```shell
sudo rosdep init
rosdep update
rosdep install --from-paths src --ignore-src -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers"
```

### 安装额外的中间件（可选）

参考文档

* [Working with multiple ROS 2 middleware implementations](https://docs.ros.org/en/jazzy/How-To-Guides/Working-with-multiple-RMW-implementations.html)

### 在工作文件夹内构建源码

如果之前已经通过别的方法安装了`ROS2`(比如通过二进制分发的方式)，确保之后的命令运行在一个干净的环境中。

确保在环境变量文件`.bashrc` 中没有`source /opt/ros/${ROS_DISTRO}/setup.bash`，可以通过以下命令检测

```shell
printenv | grep -i ROS
```

输出的结果应该为空。

进入之前存储源码的工作文件夹

```shell
cd ~/ros2_jazzy/
colcon build --symlink-install
```

## 设置环境

```shell
# Replace ".bash" with your shell if you're not using bash
# Possible values are: setup.bash, setup.sh, setup.zsh
. ~/ros2_jazzy/install/local_setup.bash
```

## 测试一些例子

在一个终端，运行以下的`C++`代码`talker`

```shell
. ~/ros2_jazzy/install/local_setup.bash
ros2 run demo_nodes_cpp talker
```

在另一个终端，运行以下的`Python`代码`listener`

```shell
. ~/ros2_jazzy/install/local_setup.bash
ros2 run demo_nodes_py listener
```

应该可以看到`talker`说`it’s Publishing messages`，`listener`说`I heard those messages.`.
