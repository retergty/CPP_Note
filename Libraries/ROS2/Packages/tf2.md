# tf2

参考文档

* [tf2](https://docs.ros.org/en/humble/Tutorials/Intermediate/Tf2/Tf2-Main.html)

一个机器人系统通常包括许多三维坐标系，这些坐标系通常是时变的，比如世界坐标系，基座坐标系，抓手坐标系，头部坐标系等等。`tf2`会跟踪这些在坐标系，并允许用户询问

* `5`秒前，头部坐标系相对于世界坐标系在哪里？
* 抓手坐标系相对于基座坐标系的姿态是什么？
* 在地图坐标系下，当前基座坐标系的姿态是什么？

## 主要概念

### 广播

`tf2`采用广播`broadcaster`来发布指定坐标系的信息，在某一时刻，广播指定的坐标系`id`，子坐标系`id`，它相对于父坐标系的姿态。

### 静态广播

有的坐标系相对于其父坐标系是恒定不变的，比如固连在机体上的摄像头坐标系，`tf2`支持发布静态坐标系广播。

### 动态广播

有的坐标系随着时间改变，那么便需要在坐标系改变时发布`tf2`广播，只有发布了广播，此时的坐标系才会被`tf2`获取到，从而用于后续的坐标系转换。

### tf2树

`tf2`给管理的坐标系构建了树形结构，且不允许闭环的树。

![tf2 tree](../Picture/turtlesim_frames.png)

`tf2`树使得`tf2`可以很方便地管理许多坐标系，并在需要获取某一坐标系的时候沿着树回溯，最终获取到当前某一坐标系相对于其它任何坐标系的姿态。

注意，`tf2`获取的坐标系信息依赖于`tf2`广播，如果一个坐标系实际改变了，但是没有使用`tf2`进行广播，`tf2`不会获取到这个改变的信息，从而使用之前的信息计算相对姿态。

很显然，如果一个坐标系的任意上游坐标系发生改变并广播，这个坐标系的绝对位置也会改变，并自动地由`tf2`计算出来。

### 查找转换`lookupTransform`

`tf2`坐标系转换会推迟到用户显示询问坐标系转换的时候，也就是用户查找转换的时候。查找转换时，还需指定查找的坐标系的时间，`tf2`会查找`tf2`树，寻找`tf2`树中时间晚于指定时间的坐标系信息，只有这个查找构成了一条从源坐标系到目标坐标系的完整的链时，转换才能成功。

```CPP
rclcpp::Time now = this->get_clock()->now();
transformStamped = tf_buffer_->lookupTransform(
   toFrameRel,
   fromFrameRel,
   now);
```

报错

```shell
[INFO] [1629873136.345688064] [listener]: Could not transform turtle2 to turtle1: Lookup would
require extrapolation into the future.  Requested time 1629873136.345539 but the latest data
is at time 1629873136.338804, when looking up transform from frame [turtle1] to frame [turtle2]
```

它告诉您该坐标系不存在或数据是未来的。

### 收听广播信息

获取`tf2`广播就是获取`tf2`树。

## tf2_ros

参考文档

* [tf2_ros](https://docs.ros2.org/foxy/api/tf2_ros/index.html)

`tf2_ros`是`C++`包，提供了`C++ API`来使用`tf2`包。

### 广播坐标系

广播坐标系的方法如下

* 构建一个`tf2_ros::TransformBroadcaster`实例
* 使用`tf2_ros::TransformBroadcaster::sendTransform()`发送一个或多个`geometry_msgs::TransformStamped`.
* 或者使用`tf2_ros::StaticTransformBroadcaster`类来发送静态广播。

### 接受广播消息

接受广播消息的方法如下

* 构建下面任意一个继承了`tf2_ros::BufferInterface`的实例
  * `tf2_ros::Buffer`是标准实现，它提供了`tf2_frames`服务,该服务可以使用`tf2_msgs::FrameGraph`响应请求。
  * `tf2_ros::BufferClient`使用`actionlib::SimpleActionClient`等待请求的转换变得可用.
    * 它应该与`tf2_ros::BufferServer`一起使用，`tf2_ros::BufferServer`提供相应的`actionlib::ActionSErver`。
* 把上一步构建的`tf2_ros::Buffer`传递给`tf2_ros::TransformListener`的构建函数。
  * 可选地，传递`rclcpp::NodeHandle`