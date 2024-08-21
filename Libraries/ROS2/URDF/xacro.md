# xacro

使用`xarco`包来简化`URDF`的编写。编写`.xarco`文件，并使用这个包来自动生成`URDF`文件。

参考文档

* [Using Xacro to clean up your code](https://docs.ros.org/en/humble/Tutorials/Intermediate/URDF/Using-Xacro-to-Clean-Up-a-URDF-File.html)
* [xacro](http://wiki.ros.org/xacro)

## 定义常量

使用如下格式即可定义常量

```xml
<xacro:property name="width" value="0.2" />
<xacro:property name="bodylen" value="0.6" />
```

之后通过`${name}`即可使用这个常量。

可以在任何`xml`语法允许的区域声明常量，并在定义之前或之后使用。

常量的展开是直接替换的，所以可以在任何地方展开。

```xml
<xacro:property name=”robotname” value=”marvin” />
<link name=”${robotname}s_leg” />
```

上述`xarco`会生成

```xml
<link name=”marvins_leg” />
```

## 计算

可以在`${}`中使用数学计算，支持的运算符为`+`,`-`,`*`,`/`,以及负号，括号。

```xml
<cylinder radius="${wheeldiam/2}" length="0.1"/>
<origin xyz="${reflect*(width+.02)} 0 0.25" />
```

## 条件判断

```xml
<xacro:if value="<expression>">
  <... some xml code here ...>
</xacro:if>
<xacro:unless value="<expression>">
  <... some xml code here ...>
</xacro:unless>
```

`<expression>`需要是`0`,`1`,`true`,`false`.

任何评估为布尔值的`python`表达式都是可行的。

```xml
<xacro:property name="var" value="useit"/>
<xacro:if value="${var == 'useit'}"/>
<xacro:if value="${var.startswith('use') and var.endswith('it')}"/>

<xacro:property name="allowed" value="${[1,2,3]}"/>
<xacro:if value="${1 in allowed}"/>
```

## 接受参数

```xml
<foo value="$(arg myvar)" />
```

可以使用调用`xacro`时的命令行参数。

也可以给它定义默认值。

```xml
<xacro:arg name="myvar" default="false"/>
```

## 宏

```xml
<xacro:macro name="default_origin">
    <origin xyz="0 0 0" rpy="0 0 0"/>
</xacro:macro>
```

定义一个名为`default_origin`的宏，之后可以在其他地方展开为指定内容。

```xml
<xacro:default_origin />
```

如果未找到指定名称的`xacro`宏，则不会展开该`xacro`，但**不会**生成错误。

## 接受参数的宏

可以编写接受参数的宏，使用宏时传递不同的值就可以展开成不同的`xml`.

```xml
<xacro:macro name="default_inertial" params="mass">
    <inertial>
            <mass value="${mass}" />
            <inertia ixx="1e-3" ixy="0.0" ixz="0.0"
                 iyy="1e-3" iyz="0.0"
                 izz="1e-3" />
    </inertial>
</xacro:macro>
```

定义了一个名为`default_inertial`的宏，接受一个名为`mass`的参数。

如果要声明多个参数，只需要使用空格隔开即可

```xml
<xacro:default_inertial mass="10"/>
```

还可以声明接受一整个块作为参数的宏。

```xml
<xacro:macro name="blue_shape" params="name *shape">
    <link name="${name}">
        <visual>
            <geometry>
                <xacro:insert_block name="shape" />
            </geometry>
            <material name="blue"/>
        </visual>
        <collision>
            <geometry>
                <xacro:insert_block name="shape" />
            </geometry>
        </collision>
    </link>
</xacro:macro>
```

声明了接受一整个块作为参数的`shape`，前面用星号`*`指明接受一整个块，并使用`<xacro:insert_block name="shape" />`展开参数块。

使用方法如下

```xml
<xacro:blue_shape name="base_link">
    <cylinder radius=".42" length=".01" />
</xacro:blue_shape>
```

## 包含其它文件

```xml
<xacro:include filename="$(find package)/other_file.xacro" />
<xacro:include filename="other_file.xacro" />
<xacro:include filename="$(cwd)/other_file.xacro" />
```

`xacro:include`用来包含其它文件，相对文件名是相对于当前处理的文件路径。

`$(find package)`获得名为`package`包的路径.

为了防止名称冲突，还可以指定名称空间。

```xml
<xacro:include filename="other_file.xacro" ns="namespace"/>
```

这样，访问宏与常量时，就需要附上名称空间

```xml
${namespace.property}
```

### 例子

```xml
<xacro:macro name="leg" params="prefix reflect">
    <link name="${prefix}_leg">
        <visual>
            <geometry>
                <box size="${leglen} 0.1 0.2"/>
            </geometry>
            <origin xyz="0 0 -${leglen/2}" rpy="0 ${pi/2} 0"/>
            <material name="white"/>
        </visual>
        <collision>
            <geometry>
                <box size="${leglen} 0.1 0.2"/>
            </geometry>
            <origin xyz="0 0 -${leglen/2}" rpy="0 ${pi/2} 0"/>
        </collision>
        <xacro:default_inertial mass="10"/>
    </link>

    <joint name="base_to_${prefix}_leg" type="fixed">
        <parent link="base_link"/>
        <child link="${prefix}_leg"/>
        <origin xyz="0 ${reflect*(width+.02)} 0.25" />
    </joint>
    <!-- A bunch of stuff cut -->
</xacro:macro>
<xacro:leg prefix="right" reflect="1" />
<xacro:leg prefix="left" reflect="-1" />
```

* 使用`name`参数声明两个相似的`link`.
* 使用`reflect`参数表示坐标系方向。
* 使用数学计算坐标系偏移。
