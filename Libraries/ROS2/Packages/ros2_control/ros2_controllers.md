# ros2 controllers

`ros2_controllers`包实现了常用的控制器。

## ros2_control_node

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

## joint_state_broadcaster

`joint_state_broadcaster`会在`CM read`时把所有的`state interfaces`关节状态都发布到`/joint_states`与`/dynamic_joint_states`.可以用来替代`joint_state_publisher`.

注意，`URDF`里定义的关节名字需要一致，否则`robot_state_publisher`不会发布正确的`tf`信息.

```python
joint_state_broadcaster_spawner = Node(
    package="controller_manager",
    executable="spawner",
    arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
)
```

### 常见参数

* `use_local_topics`,是否发布的主题前面加上所在的名称空间，比如`/my_state_broadcaster/joint_states`.默认为`false`.
