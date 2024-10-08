# ros2 controllers

`ros2_controllers`包实现了常用的控制器。

## 配置文件

配置文件会给`CM`使用来配置控制器.

例子如下

```yaml
controller_manager:
  ros__parameters:
    update_rate: 10  # Hz

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster

    diffbot_base_controller:
      type: diff_drive_controller/DiffDriveController

diffbot_base_controller:
  ros__parameters:
    left_wheel_names: ["left_wheel_joint"]
    right_wheel_names: ["right_wheel_joint"]

    wheel_separation: 0.10
    #wheels_per_side: 1  # actually 2, but both are controlled by 1 signal
    wheel_radius: 0.015

    wheel_separation_multiplier: 1.0
    left_wheel_radius_multiplier: 1.0
    right_wheel_radius_multiplier: 1.0

    publish_rate: 50.0
    odom_frame_id: odom
    base_frame_id: base_link
    pose_covariance_diagonal : [0.001, 0.001, 0.001, 0.001, 0.001, 0.01]
    twist_covariance_diagonal: [0.001, 0.001, 0.001, 0.001, 0.001, 0.01]

    open_loop: true
    enable_odom_tf: true

    cmd_vel_timeout: 0.5
    #publish_limited_velocity: true
    #velocity_rolling_window_size: 10

    # Velocity and acceleration limits
    # Whenever a min_* is unspecified, default to -max_*
    linear.x.has_velocity_limits: true
    linear.x.has_acceleration_limits: true
    linear.x.has_jerk_limits: false
    linear.x.max_velocity: 1.0
    linear.x.min_velocity: -1.0
    linear.x.max_acceleration: 1.0
    linear.x.max_jerk: 0.0
    linear.x.min_jerk: 0.0

    angular.z.has_velocity_limits: true
    angular.z.has_acceleration_limits: true
    angular.z.has_jerk_limits: false
    angular.z.max_velocity: 1.0
    angular.z.min_velocity: -1.0
    angular.z.max_acceleration: 1.0
    angular.z.min_acceleration: -1.0
    angular.z.max_jerk: 0.0
    angular.z.min_jerk: 0.0

```

## 预定义的控制器

### ros2_control_node

`ros2_control_node`里实现了一个默认的`Controller Manager`节点.

```python
robot_controllers = PathJoinSubstitution(
    [
        FindPackageShare("ros2_control_demo_example_1"),
        "config",
        "rrbot_controllers.yaml",
    ]
)
control_node = Node(
    package="controller_manager",
    executable="ros2_control_node",
    parameters=[robot_controllers],
    output="both",
    remappings=[
        ("~/robot_description", "/robot_description"),
    ],
)
```

### joint_state_broadcaster

`joint_state_broadcaster`会在`CM read`时把所有的`state interfaces`关节状态都发布到`/joint_states`与`/dynamic_joint_states`.可以用来替代`joint_state_publisher`.

注意，`URDF`里定义的关节名字需要一致，否则`robot_state_publisher`不会发布正确的`tf`信息.

```python
joint_state_broadcaster_spawner = Node(
    package="controller_manager",
    executable="spawner",
    arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
)
```

#### 常见参数

* `use_local_topics`,是否发布的主题前面加上所在的名称空间，比如`/my_state_broadcaster/joint_states`.默认为`false`.
