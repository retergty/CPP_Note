# URDF

URDF（统一机器人描述格式）是一种用于指定`ROS`中机器人的几何形状和组织的文件格式。

参考文档

* [URDF](https://docs.ros.org/en/humble/Tutorials/Intermediate/URDF/URDF-Main.html#)
* [URDF Primer](https://www.mathworks.com/help/sm/ug/urdf-model-import.html)

许多包使用`URDF`创建机器人，其中`RViz`创建了可视化的机器人，`Gazebo`对机器人进行物理学仿真。

`URDF`是用一个类XML格式的文件描述机器人的构型与特性的。

## 元素和属性

与其他类型的`XML`文件一样，`URDF`文件包含各种`XML`元素，比如`<robot>`,`<link>`,`<joint>`.有些元素可以嵌套在其他元素之内，比如`<link>`与`<joint>`可以嵌套在`<robot>`之内，作为`<robot>`的子元素，相对的`<robot>`就叫做`<link>`和`<joint>`的父元素。

```xml
<robot>
  <link>
    ...
  </link>
  <link>
    ...
  </link>
  <joint>
    ...
  </joint>
</robot>
```

子元素也可以有子元素，比如`<link>`可以有子元素`<inertial>`,`<visual>`.它们也可以有对应的子元素

```xml
<robot>
  <link>
    <inertial>
      ...
    </inertial>
    <visual>
      <geometry>
        ...
      </geometry>
      <material>
        <color />
      </material>
    </visual>
  </link>
  ...
</robot>
```

每个元素还可以有属性，比如`name`属性，表示该结构的名字，`color`表示该结构的颜色。

```xml
<robot name = "linkage">
  <link name = "root link">
    <inertial>
      ...
    </inertial>
    <visual>
      <geometry>
        ...
      </geometry>
      <material>
        <color rgba = "1 0 0 1" />
      </material>
    </visual>
  </link>
  ...
</robot>
```

## 表示连接关系

连接`<link>`通过关节`<joint>`进行连接，`<joint>`通过子元素`<parent>`来确定父连接（只能有一个），以及子元素`<child>`来确定子连接（可以有多个）。

```xml
<robot name = "linkage">
  <joint name = "joint A" >
    <parent link = "link A" />
    <child link = "link B" />
  </joint>
  <joint name = "joint B"  >
    <parent link = "link A" />
    <child link = "link C" />
  </joint>
  <joint name = "joint C" >
    <parent link = "link C" />
    <child link = "link D" />
  </joint>
</robot>
```

![kinematic tree](Picture/urdf_example_allowed_topology.png)

这样就构成了动力学树(Kinematic Tree)，动力学树不允许出现环，也就是说任何的`<link>`只能有至多一个父连接，或者说任何的`link`只能是至多一个连接的子连接。

## 可选项

不是所有的元素与属性都是必须的，一些是可选项，比如`link`下表示惯性的`<inertial>`子元素。如果可选项没有出现，那么使用`URDF`的软件可能会给它指定特定的值。

## `<robot>`

声明这个文件是`URDF`文件。

* `name`机器人的名字。

## `<link>`

参考文档

* [urdf/XML/link](https://wiki.ros.org/urdf/XML/link)

`<robot>`的子元素，添加一个新的连接。

![link](Picture/inertial.png)

* `name`连接的名字

### `<inertial>`

可选项，默认为零。

表示`<link>`的质量`mass`，质心位置以及转动惯量。

#### `<origin>`

可选项，默认为一。

表示`<link>`的质心坐标系的姿态`pose`.相对于`link`源坐标系，也可以说是连接父连接的`joint`的本地坐标系。

* `xyz`可选项，默认为零，相对坐标`xyz`.
* `rpy`可选项，默认为零，滚动角`roll angle`，俯仰角`pitch angle`，偏航角`yaw`.按照欧拉角转序`xyz`.

#### `<mass>`

`link`的质量

* `value`表示质量(kg).

#### `<inertia>`

`link`的转动惯量，绕着质心坐标系三个轴的转动惯量矩阵也就是惯性张量，默认是对称矩阵。

* `ixx`,`iyy`,`izz`对于质心坐标系`x`轴，`y`轴,`z`轴的转动惯量。
* `ixy`,`ixz`,`iyz`惯量积，最简单的方法是质心坐标系设置为与`link`的惯性主轴对齐，这样惯量积就会是零。

### `<visual>`

可选项。

表示`<link>`的可视化属性，用于可视化仿真工具，可以指定`<link>`的形状，颜色等。同一个`<link>`可以使用多次`<visual>`,它们共同构成了`<link>`的视觉属性。

* `name`可选项，指定`link`的几何图形部分的名称。这对于能够引用`link`几何形状特定部分非常有用。

#### `<origin>`

可选项，默认为一。

表示`<link>`的视觉坐标系的姿态`pose`,相对于`link`源坐标系.

* `xyz`可选项，默认为零，相对坐标`xyz`.
* `rpy`可选项，默认为零，滚动角`roll angle`，俯仰角`pitch angle`，偏航角`yaw`.按照欧拉角转序`xyz`.

#### `<geometry>`

描述`link`的视觉构型。

它可以使用以下之一的子元素描述

##### `<box>`

长方体，原点位于其中心。

* `size`长方体长宽高，比如`<box size="0.1 0.1 0.2"/>`

##### `<cylinder>`

圆柱体，原点位于底面中心。

* `radius`圆柱体半径
* `length`圆柱体高

##### `<sphere>`

球体，原点位于球心。

* `radius`球体半径

##### `<mesh>`

指定的修剪网格元素，以及可选的缩放比例。

* `filename`文件名，格式为`package://<packagename>/<path>`
* `scale`缩放比例

#### `<material>`

可选项

视觉元素的材质，允许在顶级`robot`元素中指定材质元素,在`link`内便可以通过名字指定对应材质。

* `name`材质名

##### `<color>`

可选项。

材质的颜色

* `rgba`颜色值。

##### `<texture>`

可选项

材质的纹理。

* `filename`指向纹理文件。

### `<collision>`

可选项

表示`link`的碰撞属性。会进行碰撞检测，显著提升运算时间。同一个`<link>`可以使用多次`<collision>`,它们共同构成了`<link>`的碰撞属性。

* `name`可选项，指定`link`的几何图形部分的名称。这对于能够引用`link`几何形状特定部分非常有用。

#### `<origin>`

可选项，默认为一。

表示`<link>`的碰撞体坐标系的姿态`pose`,相对于`link`源坐标系.

* `xyz`可选项，默认为零，相对坐标`xyz`.
* `rpy`可选项，默认为零，滚动角`roll angle`，俯仰角`pitch angle`，偏航角`yaw`.使用`extrinsic`旋转，欧拉角转序`xyz`.

#### `<geometry>`

描述碰撞几何体，和`<visual>`里的`<geometry>`相似。

## `<Joint>`

参考文档

* [urdf/XML/joint](https://wiki.ros.org/urdf/XML/joint)

`<robor>`的子元素，添加一个新的关节。

![joint](Picture/joint.png)

* `name`关节的名字
* `type`关节的类型，可以是以下值
  * `revolute`沿轴旋转并具有上下限的铰链关节。
  * `continuous`沿轴自由旋转，不具有上下限的铰链关节。
  * `prismatic`沿轴滑动并具有上下限滑动关节。
  * `fixed`这并不是真正的关节，因为它不能移动。不存在自由度。种类型的关节不需要`<axis>`、`<calibration>`、`<dynamics>`、`<limits>`或`<safety_controller>`。
  * `floating`允许六个自由度的关节。
  * `planar`该关节允许在垂直于轴的平面上运动。

### `<origin>`

可选项，默认为一

表示`joint`的本地坐标系的姿态，相对于父`link`的源坐标系。该坐标系也会作为子`link`的源坐标系。

* `xyz`可选项，默认为零，相对坐标`xyz`.
* `rpy`可选项，默认为零，滚动角`roll angle`，俯仰角`pitch angle`，偏航角`yaw`.使用`extrinsic`旋转，欧拉角转序`xyz`.

### `<parent>`

表示`joint`的父`link`.

* `link`父连接的名字。

### `<child>`

表示`joint`的子`link`

* `link`子连接的名字

### `<axis>`

可选项，默认为`(1,0,0)`

在`joint`的本地坐标系中指定`joint`的轴。这个轴可以是旋转轴或者是滑移轴或者是以及平面关节的法线，取决于关节类型。

* `xyz`表示关节轴，例如`<axis xyz="1 0 0"/>`

### `<calibration>`

可选项

关节的参考位置，用于校准关节的绝对位置。

* `rising`当关节正方向移动时，该参考位置将触发上升沿。
* `falling`当关节正方向移动时，该参考位置将触发下降沿。

### `<dynamics>`

可选项

指定关节动力学属性，用于物理学仿真。

* `damping`可选项，默认为零。关节的物理阻尼值，对于滑动关节，单位为`N∙s/m`；对于旋转关节，单位为`N∙m∙s/rad`.
* `friction`可选项，默认为零。关节的物理静摩擦值，对于滑动关节，单位为`N`；对于旋转关节，单位为`N∙m`.

### `<limit>`

关节限制，只对特定类型关节有意义。

* `lower`可选项，默认为零。旋转关节以`rad`为单位，对于滑动关节以`m`为单位
* `upper`可选项，默认为零。旋转关节以`rad`为单位，对于滑动关节以`m`为单位
* `effort`关节最大能输出的力。
* `velocity`关节最大移动速度，旋转关节以`rad/s`为单位，对于滑动关节以`m/s`为单位

### `<mimic>`

可选项。

指定定义的关节模拟其它存在的关节。通过公式`value = multiplier * other_joint_value + offset`.

* `joint`其它关节名字
* `multiplier`可选项
* `offset`可选项
