# Controller Manager

参考文档

* [Controller Manager](https://control.ros.org/humble/doc/ros2_control/controller_manager/doc/userdoc.html)

控制器管理器管理控制器的生命周期，访问硬件接口，提供服务。

## 参数

`Controller Manager`接受`ROS`参数如下

### hardware_components_initial_state

硬件部分的初始状态，硬件部分的名字先前通过`<ros2_control>`标签在`URDF`里声明了。默认是`activated`的，但是也可以指定为`unconfigured`,`inactive`.

```yaml
hardware_components_initial_state:
  unconfigured:
    - "arm1"
    - "arm2"
  inactive:
    - "base3"
```

### robot_description

`robot_description`描述机器人的硬件部分，通常是由自动读取`URDF`描述文件生成的。

### update_rate

`CM`更新的频率，单位为`Hz`.

### <controller_name>.type

指明控制器的类型，是一个`plugin`.
