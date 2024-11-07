# rosdep

参考文档

* [Managing Dependencies with rosdep](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Rosdep.html)

`rosdep`是依赖管理工具，可以用来管理包与外部库。注意，它不是包管理工具，它使用自己的系统知识和依赖关系来查找要安装在特定平台上的适当包。实际的安装还是由`apt`负责的。

它通常在构建工作区之前使用，用于安装在该工作区中的包依赖项。

`ros`与`rosdep`处于半不可知状态，成功地构建`ros`包不一定需要`rosdep`参与，也可以手动下载依赖。成功运行`rosdep`依赖于可用的`rosdep`密钥，可以使用几个简单的命令从公共`git`存储库下载该密钥。

## package.xml

`rosdep`使用每个包中的`package.xml`读取依赖，`package.xml`中的依赖项列表完整且正确性至关重要。

## 工作原理

`rosdep`会检查当前工作区包的`package.xml`,并发现其中存储的`ros`包关键字。这些关键字与中央索引(central index)交叉引用，以在各种包管理器中找到适当的`ROS`包或软件库。

`rosdep`检索本地计算机上的中央索引，这样它就不必在每次运行时访问网络,在`Ubuntu`中，配置文件存储在`/etc/ros/rosdep/sources.list.d/20-default.list`

中央索引称为`rosdistro`,是与互联网交互找到的。

## 获取ros关键字

* 如果依赖的包是`ROS`包，**且**这个包已经发布到`ROS`生态系统(ecosystem)中，比如`nav2_bt_navigator`包。发布到`ROS`生态系统意味着这个包在[rosdistro database数据库](https://github.com/ros/rosdistro)中的`<distro>/distribution.yaml`文件中有这个包的名称。我们可以在对应的`<distro>/distribution.yaml`找到当前`ROS`版本生态系统中所有的包。
* 如果依赖的包不是`ROS`包，这种依赖也叫做系统依赖(system dependencies),也需要找到特定的关键字，[rosdep/base.yaml](https://github.com/ros/rosdistro/blob/master/rosdep/base.yaml)`apt`管理系统依赖，[rosdep/python.yaml](https://github.com/ros/rosdistro/blob/master/rosdep/python.yaml)管理`Python`依赖。

比如，假设工程依赖一个非ROS包`doxygen`,我们在`rosdep/base.yaml`中搜索`doxygen`发现

```yaml
doxygen:
  arch: [doxygen]
  debian: [doxygen]
  fedora: [doxygen]
  freebsd: [doxygen]
  gentoo: [app-doc/doxygen]
  macports: [doxygen]
  nixos: [doxygen]
  openembedded: [doxygen@meta-oe]
  opensuse: [doxygen]
  rhel: [doxygen]
  ubuntu: [doxygen]
```

这意味着我们的`rosdep`关键字是`doxygen`，`rosdep`会按照不同的系统架构解析成对应在特定系统的，用于`apt install`的包名。

## 用法

### 安装

使用如下方法安装`rosdep`工具

```shell
apt-get install python3-rosdep
```

### 初始化

首次使用`rosdep`时需要初始化，这也是唯一需要使用`sudo`修饰的情况

```shell
sudo rosdep init
```

### 更新本地索引

由于`rosdep`查找包时是通过本地索引，所以有必要周期性地更新本地索引

```shell
rosdep update
```

### 安装依赖包

在需要安装依赖包的工作区

```shell
rosdep install --from-paths src -y --ignore-src
rosdep install -r --from-paths src --ignore-src --rosdistro humble -y
```

* `--from-paths src`表示在子目录`src`中查找递归查找`package.xml`解决未出现的依赖
* `-y`意味着所有的安装讯问默认为`yes`.
* `--ignore-src`忽略已安装的包，也就是说`rosdep`会忽略通过环境变量`ROS_PACKAGE_PATH`与`AMENT_PREFIX_PATH`与`--from-paths`找到的包关键字，这些包关键字视为已安装。
* `-r`
