# launch

参考文档

* [launch](https://docs.ros.org/en/humble/Tutorials/Intermediate/Launch/Launch-Main.html)

`ROS2 launch`文件允许我们可以同时配置并启动一系列包含`ROS2`节点的可执行文件。

`ROS2 launch`文件帮助用户描述可执行文件与节点的配置，比如描述要运行的程序，程序的位置，传递的参数。`ROS2 launch`系统还可以监测程序运行状态并报告。

## 创建并运行launch文件

在想创建`launch`文件的包中

```shell
mkdir launch
cd launch
```

在`launch`中，创建一个`launch`文件，以`turtlesim_mimic_launch.py`为例。

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim'
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim'
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ]
        )
    ])
```

上面的`launch`文件启动一个含有三个节点的系统，这三个节点都是包`turtlesim`里的。这个文件的目的是创建两个`turtle`,并让其中一个`turtle`跟随另一个`turtle`的移动。

当启动两个`turtle`节点时，他们唯一的不同是它们的名称空间`namespace`,指定不同的名称空间可以使得两个节点没有名称冲突与主题名冲突(创建的节点名与主题都会加上名称空间的前缀)。

第三个节点是`mimic`可执行文件，该节点以重新映射的形式添加了配置详细信息，`mimic`的`/input/pose`被重映射为`/turtlesim1/turtle1/pose`,`/output/cmd_vel`被重映射为`/turtlesim2/turtle1/cmd_vel`，也就是按照`/turtlesim1/turtle1`的`pose`姿态，给`/turtlesim2/turtle1`发送指令。

在`CMakeLists.txt`中加入

```CMake
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

把`launch`文件安装到指定位置。

在`<package.xml>`添加

```xml
<exec_depend>ros2launch</exec_depend>
```

## 写法

### 引入的包

```python
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from launch.actions import DeclareLaunchArgument, ExecuteProcess, TimerAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
```

### 格式

```python
def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim'
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim'
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ]
        )
    ])
```

* 定义一个函数`generate_launch_description`
* 这个函数返回`LaunchDescription`实例
* `LaunchDescription`实例包含`Node`字段，一个`Node`就是一个可执行文件。

### 使用参数

```python
from launch_ros.actions import Node

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, TimerAction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PythonExpression


def generate_launch_description():
    turtlesim_ns = LaunchConfiguration('turtlesim_ns')
    use_provided_red = LaunchConfiguration('use_provided_red')
    new_background_r = LaunchConfiguration('new_background_r')

    turtlesim_ns_launch_arg = DeclareLaunchArgument(
        'turtlesim_ns',
        default_value='turtlesim1'
    )
    use_provided_red_launch_arg = DeclareLaunchArgument(
        'use_provided_red',
        default_value='False'
    )
    new_background_r_launch_arg = DeclareLaunchArgument(
        'new_background_r',
        default_value='200'
    )

    turtlesim_node = Node(
        package='turtlesim',
        namespace=turtlesim_ns,
        executable='turtlesim_node',
        name='sim'
    )
    spawn_turtle = ExecuteProcess(
        cmd=[[
            'ros2 service call ',
            turtlesim_ns,
            '/spawn ',
            'turtlesim/srv/Spawn ',
            '"{x: 2, y: 2, theta: 0.2}"'
        ]],
        shell=True
    )
    change_background_r = ExecuteProcess(
        cmd=[[
            'ros2 param set ',
            turtlesim_ns,
            '/sim background_r ',
            '120'
        ]],
        shell=True
    )
    change_background_r_conditioned = ExecuteProcess(
        condition=IfCondition(
            PythonExpression([
                new_background_r,
                ' == 200',
                ' and ',
                use_provided_red
            ])
        ),
        cmd=[[
            'ros2 param set ',
            turtlesim_ns,
            '/sim background_r ',
            new_background_r
        ]],
        shell=True
    )

    return LaunchDescription([
        turtlesim_ns_launch_arg,
        use_provided_red_launch_arg,
        new_background_r_launch_arg,
        turtlesim_node,
        spawn_turtle,
        change_background_r,
        TimerAction(
            period=2.0,
            actions=[change_background_r_conditioned],
        )
    ])
```

* `LaunchConfiguration`替换函数获取指定参数的启动配置，这个获取函数会在用到时才获取。
* `DeclareLaunchArgument`声明了一个参数，这个参数的名字与默认值，之后需要传递给`LaunchDescription`.
* `Node`获取`turtlesim_ns`的配置
* `ExecuteProcess`执行一段程序，由`cmd`指定的程序，具体为它们字符串的拼接值。
* `IfCondition`判断条件是否满足，如果满足才会执行任务。
* `TimerAction`创建了一个时间任务

这段启动文件按顺序做了

1. 运行了一个`Node`.
2. 生成第二个`turtle`.
3. 将背景改成紫色
4. 如果参数通过条件，那么把背景改为粉色，两秒后。

可以给这个启动文件传递参数

```python
ros2 launch launch_tutorial example_substitutions.launch.py turtlesim_ns:='turtlesim3' use_provided_red:='True' new_background_r:=200
```

### 注册事件回调

参考文档

* [Using event handlers](https://docs.ros.org/en/humble/Tutorials/Intermediate/Launch/Using-Event-Handlers.html)

### 管理大工程

参考文档

* [Managing large projects](https://docs.ros.org/en/humble/Tutorials/Intermediate/Launch/Using-ROS2-Launch-For-Large-Projects.html)

大工程通常有复杂的节点，所以启动文件也会分层。

#### 顶层`launch_turtlesim.launch.py`文件

```python
import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
   turtlesim_world_1 = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/turtlesim_world_1.launch.py'])
      )
   turtlesim_world_2 = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/turtlesim_world_2.launch.py'])
      )
   broadcaster_listener_nodes = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/broadcaster_listener.launch.py']),
      launch_arguments={'target_frame': 'carrot1'}.items(),
      )
   mimic_node = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/mimic.launch.py'])
      )
   fixed_frame_node = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/fixed_broadcaster.launch.py'])
      )
   rviz_node = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('learning_launch'), 'launch'),
         '/turtlesim_rviz.launch.py'])
      )

   return LaunchDescription([
      turtlesim_world_1,
      turtlesim_world_2,
      broadcaster_listener_nodes,
      mimic_node,
      fixed_frame_node,
      rviz_node
   ])
```

这个启动文件包含一系列子启动文件，每个这些子启动文件都可能包含节点，参数，或者是递归地包含启动文件。

* `IncludeLaunchDescription`函数把一个新的`launch`文件包含进来。
* `PythonLaunchDescriptionSource`函数接收这个`launch`文件的地址
* `os.path.join`自动按照操作系统文件路径把字符串拼接起来。
* `get_package_share_directory`获取指定包的文件路径。
* `items`函数返回字典的列表。

#### `turtlesim_world_1.launch.py`文件

```python
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, TextSubstitution

from launch_ros.actions import Node


def generate_launch_description():
   background_r_launch_arg = DeclareLaunchArgument(
      'background_r', default_value=TextSubstitution(text='0')
   )
   background_g_launch_arg = DeclareLaunchArgument(
      'background_g', default_value=TextSubstitution(text='84')
   )
   background_b_launch_arg = DeclareLaunchArgument(
      'background_b', default_value=TextSubstitution(text='122')
   )

   return LaunchDescription([
      background_r_launch_arg,
      background_g_launch_arg,
      background_b_launch_arg,
      Node(
         package='turtlesim',
         executable='turtlesim_node',
         name='sim',
         parameters=[{
            'background_r': LaunchConfiguration('background_r'),
            'background_g': LaunchConfiguration('background_g'),
            'background_b': LaunchConfiguration('background_b'),
         }]
      ),
   ])
```

* 使用`parameters`给节点传递参数。

#### `turtlesim_world_2.launch.py`文件

```python
import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
   config = os.path.join(
      get_package_share_directory('learning_launch'),
      'config',
      'turtlesim.yaml'
      )

   return LaunchDescription([
      Node(
         package='turtlesim',
         executable='turtlesim_node',
         namespace='turtlesim2',
         name='sim',
         parameters=[config]
      )
   ])
```

这个文件同样启动了一个`turtle`节点，只不过它的名称空间不同，而且是读取`YAML`文件作为参数的。

在`config`文件夹中，创建对应的`turtlesim.yaml`文件

```yaml
/turtlesim2/sim:
   ros__parameters:
      background_b: 255
      background_g: 86
      background_r: 150
```

#### `turtlesim_world_3.launch.py`文件

```python
import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
   config = os.path.join(
      get_package_share_directory('learning_launch'),
      'config',
      'turtlesim.yaml'
      )

   return LaunchDescription([
      Node(
         package='turtlesim',
         executable='turtlesim_node',
         namespace='turtlesim3',
         name='sim',
         parameters=[config]
      )
   ])
```

同时在`turtlesim.yaml`中添加通配符，是的所有未匹配的都会使用通配符

```yaml
/**:
   ros__parameters:
      background_b: 255
      background_g: 86
      background_r: 150
```

为每个节点指定名称空间是很麻烦的，所以，最好在顶层指定一个子启动文件的前缀名称空间。

在顶层启动文件`launch_turtlesim.launch.py`添加以下行

```python
from launch.actions import GroupAction
from launch_ros.actions import PushRosNamespace

   ...
   turtlesim_world_2 = IncludeLaunchDescription(
      PythonLaunchDescriptionSource([os.path.join(
         get_package_share_directory('launch_tutorial'), 'launch'),
         '/turtlesim_world_2.launch.py'])
      )
   turtlesim_world_2_with_namespace = GroupAction(
     actions=[
         PushRosNamespace('turtlesim2'),
         turtlesim_world_2,
      ]
   )
```

并在`LaunchDescription`中把`turtlesim_world_2`替换为`turtlesim_world_2_with_namespace`.

这样，所有通过`turtlesim_world_2.launch.py`启动的文件都会具有名称空间`turtlesim2`.

#### `broadcaster_listener.launch.py`文件

```python
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def generate_launch_description():
   return LaunchDescription([
      DeclareLaunchArgument(
         'target_frame', default_value='turtle1',
         description='Target frame name.'
      ),
      Node(
         package='turtle_tf2_py',
         executable='turtle_tf2_broadcaster',
         name='broadcaster1',
         parameters=[
            {'turtlename': 'turtle1'}
         ]
      ),
      Node(
         package='turtle_tf2_py',
         executable='turtle_tf2_broadcaster',
         name='broadcaster2',
         parameters=[
            {'turtlename': 'turtle2'}
         ]
      ),
      Node(
         package='turtle_tf2_py',
         executable='turtle_tf2_listener',
         name='listener',
         parameters=[
            {'target_frame': LaunchConfiguration('target_frame')}
         ]
      ),
   ])
```

创建了两次`turtle_tf2_broadcaster`,但是接受的参数不一样，意味着广播的`turtle`坐标不同。

注意在顶层时，我们`launch_arguments={'target_frame': 'carrot1'}.items()`覆盖了参数。

#### `mimic.launch.py`文件

```python
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
   return LaunchDescription([
      Node(
         package='turtlesim',
         executable='mimic',
         name='mimic',
         remappings=[
            ('/input/pose', '/turtle2/pose'),
            ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
         ]
      )
   ])
```

* `remappings`重新定义主题名。

#### `turtlesim_rviz.launch.py`文件

```python
import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
   rviz_config = os.path.join(
      get_package_share_directory('turtle_tf2_py'),
      'rviz',
      'turtle_rviz.rviz'
      )

   return LaunchDescription([
      Node(
         package='rviz2',
         executable='rviz2',
         name='rviz2',
         arguments=['-d', rviz_config]
      )
   ])
```

启动了`rviz2`.

#### `fixed_broadcaster.launch.py`文件

```python
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import EnvironmentVariable, LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
   return LaunchDescription([
      DeclareLaunchArgument(
            'node_prefix',
            default_value=[EnvironmentVariable('USER'), '_'],
            description='prefix for node name'
      ),
      Node(
            package='turtle_tf2_py',
            executable='fixed_frame_tf2_broadcaster',
            name=[LaunchConfiguration('node_prefix'), 'fixed_broadcaster'],
      ),
   ])
```

从环境中读取环境变量`USER`并添加`_`，最后节点的名字便是`${USER}_fixed_broadcaster`.

## 常见参数

### Node

* `output='screen'`表示把输出定位到屏幕里。
* `package='turtlesim'`节点所在的包
* `executable='turtlesim_node'`包里的可执行文件名
* `name='sim'`把节点的名字修改为`sim`,如果可执行文件里有多个节点，会修改所有节点的名称。
* `parameters`表示节点要接受的参数。
* `namespace='turtlesim2'`把节点的名称空间修改为`turtlesim2`,如果可执行文件里有多个节点，会修改所有节点的名称空间。
* `remappings`修改主题名。
* `arguments`修改传入的参数。

### ComposableNodeContainer

* `name='my_container'`节点容器重命名
* `namespace=''`节点容器名称空间重新设置
* `package='rclcpp_components'`节点容器所在的包
* `executable='component_container'`节点容器所在的可执行文件。
* `output='screen'`表示把输出定位到屏幕里。
* `composable_node_descriptions`接受组合节点描述符
  * `package`组合节点所在的包名
  * `plugin`组合节点，也是组件名，之前在`CMakeLists.txt`中注册的组件名
  * `name`组合节点重命名为。

   ```python
   composable_node_descriptions=[
      ComposableNode(
         package='composition',
         plugin='composition::Talker',
         name='talker'),
      ComposableNode(
         package='composition',
         plugin='composition::Listener',
         name='listener')
   ],
   ```

### DeclareLaunchArgument

```python
turtlesim_ns_launch_arg = DeclareLaunchArgument(
   'turtlesim_ns',
   default_value='turtlesim1'
)
```

* `'turtlesim_ns'`是启动参数的名字，可以与其它变量同名。
* `default_value='turtlesim1'`默认值

### ExecuteProcess

```python
spawn_turtle = ExecuteProcess(
   cmd=[[
      'ros2 service call ',
      turtlesim_ns,
      '/spawn ',
      'turtlesim/srv/Spawn ',
      '"{x: 2, y: 2, theta: 0.2}"'
   ]],
   shell=True
)
```

* `cmd`接收一个`shell`命令.
* `shell=True`使用`shell`.

### LaunchDescription

`LaunchDescription`接受一个列表，这个列表的每个元素都是`launch_ros.actions`.

* `Node`最常用的动作，启动一个节点。
* `ComposableNodeContainer`启动一个节点容器
* `DeclareLaunchArgument`声明一个`launch`参数
* `ExecuteProcess`执行一段`shell`命令
