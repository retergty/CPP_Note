# mujoco

参考文档

* [官方参考文档](https://mujoco.readthedocs.io/en/stable/overview.html)
* [官方github库](https://github.com/google-deepmind/mujoco)

`mujoco`是强化学习中常用的仿真器，通常用于`sim-to-sim`.`MuJoCo`是一个`C/C++`库，带有`C API`，面向研究人员和开发人员。模型使用自定义的`MJCF`的`xml`语言,也可以使用`urdf`文件。

## 关键特性

* 广义坐标与现代接触动力学相结合
* 软性、凸性和解析可逆的接触动力学
* 肌腱几何结构
* 通用驱动模型
* 可重构计算管道
* 模型编译
* 模型与数据的分离
* 交互式模拟和可视化
* 功能强大且直观的建模语言
* 自动生成复合柔性对象

## 模型实例

模型`models`使用语法为`MJCF`或者是`URDF`的`xml`.之后`mujoco`可以使用这个文件创造多个结构，如下

|  |顶层| 地层 | 
|-------|-------|-------| 
| **文件** | MJCF/URDF (XML) | MJB (binary) |
| **内存** | mjSpec (C struct) | mjModel (C struct) |

所有的运行时计算使用`mjModel`进行，但是它过于复杂，由`xml`文件自动生成.

C语言结构体风格的`mjSpec`与`xml`文件一一对应.`xml`加载器将`xml`翻译为对应的`mjSpec`并最终编译为`mjModel`.

* (文本编辑器) → MJCF/URDF文件→ (MuJoCo parser → mjSpec → compiler) → mjModel

* (用户代码) → mjSpec → (MuJoCo compiler) → mjModel

* MJB文件 → (model loader) → mjModel

### 例子

```xml
<mujoco>
  <worldbody>
    <light diffuse=".5 .5 .5" pos="0 0 3" dir="0 0 -1"/>
    <geom type="plane" size="1 1 0.1" rgba=".9 0 0 1"/>
    <body pos="0 0 1">
      <joint type="free"/>
      <geom type="box" size=".1 .2 .3" rgba="0 .9 0 1"/>
    </body>
  </worldbody>
</mujoco>
```

这个例子定义了一个地平面，与一个漂浮起来的长方体，有`6`自由度(`joint`的类型是`free`).

<div style="text-align: center;">
    <img src="./picture/model_example.png" alt="模型例子" width="200" height="200" style="额外样式">
</div>

如果开始仿真,这个长方体会掉落到地面上.

下面给出的是无渲染的被动动力学基本仿真代码。

```CPP
#include "mujoco.h"
#include "stdio.h"

char error[1000];
mjModel* m;
mjData* d;

int main(void) {
  // load model from file and check for errors
  m = mj_loadXML("hello.xml", NULL, error, 1000);
  if (!m) {
    printf("%s\n", error);
    return 1;
  }

  // make data corresponding to model
  d = mj_makeData(m);

  // run simulation for 10 seconds
  while (d->time < 10)
    mj_step(m, d);

  // free model and data
  mj_deleteData(d);
  mj_deleteModel(m);

  return 0;
}
```

## 模型元素

本节描述了mujoco模型要使用的所有元素.

### Options

每个模型都有如下三个可配置的选项组。如果xml文件中没有描述到，就会给它们一个默认值。这些选项可以在每个模拟时间步之前更改。但是，在一个时间步内，任何选项都不应更改。

#### mjOption

该组包含所有影响物理模拟的选项。它用于选择算法并设置其参数，启用和禁用模拟管道的不同部分，以及调整系统级物理属性，例如重力。

```CPP
struct mjOption_ {                // physics options
  // timing parameters
  mjtNum timestep;                // timestep

  // solver parameters
  mjtNum impratio;                // ratio of friction-to-normal contact impedance
  mjtNum tolerance;               // main solver tolerance
  mjtNum ls_tolerance;            // CG/Newton linesearch tolerance
  mjtNum noslip_tolerance;        // noslip solver tolerance
  mjtNum ccd_tolerance;           // convex collision solver tolerance
  ...
}
```

#### mjVisual

该组包含所有可视化选项，还有其他`OpenGL`渲染选项，但这些选项与会话相关，不属于该模型的一部分。

```CPP
struct mjVisual_ 
{                // visualization options
  struct {                        // global parameters
    int   cameraid;               // initial camera id (-1: free)
    int   orthographic;           // is the free camera orthographic (0: no, 1: yes)
    float fovy;                   // y field-of-view of free camera (orthographic ? length : degree)
    float ipd;                    // inter-pupilary distance for free camera
    float azimuth;                // initial azimuth of free camera (degrees)
    float elevation;              // initial elevation of free camera (degrees)
    float linewidth;              // line width for wireframe and ray rendering
    float glow;                   // glow coefficient for selected body
    float realtime;               // initial real-time factor (1: real time)
    int   offwidth;               // width of offscreen buffer
    int   offheight;              // height of offscreen buffer
    int   ellipsoidinertia;       // geom for inertia visualization (0: box, 1: ellipsoid)
    int   bvactive;               // visualize active bounding volumes (0: no, 1: yes)
  } global;
  ...
}
```

#### mjStatistic

该结构体包含编译器计算出的模型统计信息,比如质量，模型的空间范围等，

```CPP
struct mjStatistic_ {             // model statistics (in qpos0)
  mjtNum meaninertia;             // mean diagonal inertia
  mjtNum meanmass;                // mean body mass
  mjtNum meansize;                // mean body size
  mjtNum extent;                  // spatial extent
  mjtNum center[3];               // center of model
};
```

### Assets

Assets本身不是模型元素，但是模型元素可以引用它，比如导入一个Mesh等。

#### Mesh

#### Skin

#### Texture

#### Material

### Kinematic tree

Kinematic tree,动力学树把所有要仿真的物体联系起来，添加约束。树的结构通过`mjModel.body_parentid`,这是一个整数数组，长度为`nbody`.注意，最顶层的`world`物体总是存在的且`id`为`0`,(`body_parentid[0] == 0`).对于其它的物体，总是有`body_parentid[i] < i`.注意，`world`物体和其他静态（无关节）子物体构成一个独特的“静态树”，没有相关的自由度。

动力学树禁止出现环，如果需要环，使用等式约束来建模。因此，`MuJoCo`模型的主干是由嵌套的物体定义形成的一个或多个运动学树,一个孤立的漂浮物体也算作一棵树。

下面列出的元素都是在某个物体内定义的，属于这个物体。注意和后面的`stand-alone`元素区别开.

#### Body

物体具有质量和惯性，但不具有几何特性。相反，几何特性被附加到物体上。

每个物体都有两个坐标系：一个是定义它的坐标系，其它元素相对与他的坐标，一个是它内部的惯性坐标系，中心在质心处，方向与物体的主轴方向一致（所以物体的惯性张量是对角矩阵）。

在每个时间步，`MuJoCo`递归地计算正向运动学，得到所有物体在全局笛卡尔坐标系中的位置和姿态。这为所有后续计算奠定了基础。物体的数量由`mjModel.nbody`给出。

#### Joint

关节定义在物体内，它表示了当前物体与父物体间的自由度，共有四种类型的关节：球关节 (ball)、滑动关节 (slide)、铰链关节 (hinge)，以及一种用于创建浮动刚体的“自由关节” (free joint)。

单个 Body 可以拥有多个关节。通过这种方式，可以自动创建复合关节，而无需定义“虚拟刚体”（dummy bodies）。

球关节和自由关节的方向分量使用单位四元数表示，MuJoCo 中的所有计算都遵循四元数的数学性质。

关节的总数由`mjModel.njnt`给出。

#### Joint reference

`mjModel.qpos0`中存储了所有关节的参考位置，每当仿真重置（reset）时，关节位置`mjData.qpos`就会被设定为`mjModel.qpos0`。在运行时，，关节位置向量是相对于参考位姿来解释的。也就是说，由关节施加的空间变换量是`mjData.qpos - mjModel.qpos0`。这种变换是在`mjModel`的`body`元素中存储的父子平移和旋转偏移量之外附加的。

这个属性只能影响标量关节，比如`siide`与`hinge`.对于球关节，`mjModel.qpos0`里永远是单位四元数.对于自由关节，`mjModel.qpos0`里面存储的是全局坐标与四元数.

#### Spring reference

弹簧参考位姿，在这个位姿下，所有的关节弹簧和肌腱弹簧都处于静息长度（resting length）。

当关节构型偏离弹簧参考位姿时，就会产生弹力。弹力的大小与偏离量成线性关系。

弹簧参考位姿保存在`mjModel.qpos_spring`中.

对于滑动关节（slide）和铰链关节（hinge），弹簧参考值是通过属性`springref`指定的。

对于球关节（ball）和自由关节（free），弹簧参考位姿对应于模型的初始构型。

#### DOF

自由度 (DOFs) 与关节密切相关，但它们并非一一对应的，因为球关节（ball joints）和自由关节（free joints）拥有多个自由度。

可以将关节理解为指定**位置**信息的元素，而将自由度理解为指定**速度**和**力**信息的元素。换句话说，关节位置是系统构型流形 (configuration manifold) 上的坐标，而关节速度则是该流形在当前位置的切空间 (tangent space) 上的坐标。

自由度具有与速度相关的属性，例如摩擦损耗 (friction loss)、阻尼 (damping) 和电枢惯量 (armature inertia)。作用于系统的所有广义力都是在自由度空间中表示的。

相比之下，关节具有与位置相关的属性，例如限位 (limits) 和弹簧刚度 (spring stiffness)。

自由度并非由用户直接指定。相反，它们是由编译器根据（用户定义的）关节自动创建的。自由度的数量由 `mjModel.nv`给出。

##### 关节和自由度的区别

* 关节(Joint)是位置域：
  * 对应代码中的`mjModel.nq`
  * 数据存储在`data.qpos`中
  * 属性： 限位、刚度
* 自由度(DOF)是速度域/力域
  * 对应代码中的`mjModel.nv`
  * 数据存储在`data.qvel`(速度) 和`data.qfrc_applied`(力) 中。
  * 属性： 摩擦(动得越快阻力越大)、阻尼、惯量

##### 构型流形

在刚体动力学中，机器人的所有可能位置和姿态组成的集合，称为构型空间（Configuration Space）。在数学上，这是一个流形（Manifold）。

流形（Manifold） 是一个在局部（Local） 看起来像通常的欧几里得空间（$\mathbb{R}^n$），但在全局（Global） 上具有不同拓扑结构的数学空间。比如球面（Sphere, $S^2$），就是一个流形，在局部近似为二维平面 $\mathbb{R}^2$

#### Tree

一个动力学树就是由一个可移动刚体及其所有后代组成。世界体（worldbody）和其他静态刚体虽然处于全局树结构中，但不从属于任何（运动学）“树”。

全局树结构使用深度优先（depth-first）的组织方式，属于同一棵树的所有刚体、关节和自由度（DOFs）在内存中总是连续排列的。

刚体（如果是静态的）可能不属于任何树，但关节和自由度总是属于某一棵树（因为有关节意味着可移动）。

动力学树的数量由`mjModel.ntree`给出。例如，一个包含三个自由刚体（free bodies）和一个标准人形机器人的模型，其`ntree = 4`。

虽然这些“树”确实是全局树（以 world 为根）的子树，但这不应与 `MuJoCo` 中的专用术语子树（subtree）混淆,“子树”一词专指以每个刚体为根的局部树。

因此`mjModel.body_subtreemass`给出了每个刚体下方局部树的总质量（该属性适用于所有刚体）。

#### Geom

几何体（Geoms） 是刚性附着在刚体（bodies）上的 3D 形状，同一个刚体上可以附着多个几何体。

`MuJoCo`仅支持凸几何体（convex geom）之间的碰撞，而创建**非凸物体（non-convex objects）**的唯一方法，就是将其表示为多个凸几何体的组合（union）。

除了用于碰撞检测和随后的接触力计算外，几何体还用于渲染（可视化），以及在省略刚体质量和惯量参数时，用于自动推断刚体的质量和惯性矩阵。

MuJoCo 支持多种基础几何形状：平面 (plane)、球体 (sphere)、胶囊体 (capsule)、椭球体 (ellipsoid)、圆柱体 (cylinder) 和长方体 (box)。

几何体也可以是网格（mesh）或高度场（height field）；这通过引用相应的资源（asset）来实现。

几何体拥有许多材质属性，这些属性会影响仿真物理效果和视觉显示。

几何体的总数由`mjModel.ngeom`给出。

#### Site

Site（站点） 本质上是轻量级的几何体（light geoms）。它们代表了刚体坐标系内感兴趣的位置（locations of interest）。

`Site`不参与碰撞检测，也不参与惯性属性的自动计算。

它们可用于指定其他对象的空间属性，例如传感器（sensors）、肌腱布线（tendon routing）以及曲柄滑块机构的端点（slider-crank endpoints）。

`Site`的总数由`mjModel.nsite`给出。

#### Camera

一个模型中可以定义多个摄像机。始终存在一个默认摄像机，用户可以在交互式可视化器中用鼠标自由移动它。

然而，定义额外的摄像机通常很方便，这些摄像机既可以是固定在世界坐标系中的（类似监控探头），也可以是附着在某个刚体上并随之移动的（类似第一人称视角）。可以用来生成图像数据.

除了摄像机的位置和方向外，用户还可以调整垂直视场角（Field of View, FOV）和用于立体渲染的瞳距（inter-pupilary distance），以及创建立体虚拟环境所需的倾斜投影（oblique projections）。

当模拟具有非完美光学特性的真实摄像机时，可以为水平和垂直方向指定单独的焦距，以及非中心的主点（principal point）。

摄像机的数量由`mjModel.ncam`给出。

#### Light

灯光（Lights） 可以固定在世界刚体（world body）上，也可以附着在移动刚体上。

可视化器提供了对 OpenGL（固定管线） 完整光照模型的访问，包括环境光（ambient）、漫反射（diffuse）和镜面反射（specular）分量，以及衰减（attenuation）、截止（cutoff）、位置和定向光照、雾效（fog）。

请注意，除了用户在运动学树中定义的灯光外，还有一个默认的头灯（headlight），它会随摄像机移动。其属性可以通过`mjVisual`选项进行调整。

灯光的数量由`mjModel.nlight`给出。

### Stand-alone

这里描述的模型元素不属于独立的刚体，所以不在动力学树内描述。

#### Tendon

Tendon（肌腱/绳索）是标量长度元件，可用于致动（actuation）、施加限位和等式约束，或者创建弹簧-阻尼器及摩擦损耗。

肌腱主要分为两种类型：固定肌腱（fixed）和空间肌腱（spatial）。

* 固定肌腱是（标量）关节位置的线性组合。它们对于模拟机械耦合非常有用。

* 空间肌腱被定义为穿过一系列指定站点（或途经点 via-points），或者包裹（wrap）在指定几何体周围的最短路径。

仅支持**球体（spheres）和圆柱体（cylinders）**作为包裹几何体，且在计算包裹时，圆柱体被视为具有无限长度。

##### 固定肌腱 (Fixed Tendons)

固定肌腱 (`Fixed Tendons`)是`MuJoCo`的一种抽象表达。它没有物理上的“绳子”，它只是一个数学公式：

$$
L = c_0 + c_1 q_1 + c_2 q_2 + \dots
$$

其中 $q_i$ 是关节角度。

* 齿轮箱：如果你想让关节 B 的转速永远是关节 A 的 2 倍，可以定义一个固定肌腱，让 $L = 2 q_A - q_B$，然后把 $L$ 约束为常数（Equality Constraint）。

##### 空间肌腱 (Spatial Tendons)

空间肌腱 (Spatial Tendons)是真正的物理模拟，涉及到复杂的几何计算（最短路径问题）。

* 路径规划 (Routing)：
  * 肌腱由一系列 site 连接而成。
  * 果在两个 site 之间放置了一个障碍物（Geom），肌腱会自动“贴”在障碍物表面，就像绳子绕过滑轮一样。

* 包裹限制 (The Wrapping Limit)：
  * `MuJoCo`只允许肌腱绕过球体和圆柱体(计算线段到球体/圆柱体的切点有解析解（Analytic Solution），速度极快。如果支持任意 Mesh 的包裹，就需要迭代求解，会导致仿真变慢甚至卡死)。

#### Actuator

MuJoCo 提供了一个灵活的执行器模型，它由三个可以独立指定的组件组成。它们共同决定了执行器的工作方式。

常见的执行器类型是通过以协调的方式指定这些组件而获得的。

这三个组件分别是：传输（Transmission）、激活动力学（Activation Dynamics）和力生成（Force Generation）。

* 传输指定了执行器如何连接到系统的其余部分；可用的类型包括关节（joint）、肌腱（tendon）和曲柄滑块（slider-crank）。

* 激活动力学可用于模拟气动或液压缸以及生物肌肉的内部激活状态；使用此类执行器会使整体系统动力学变为三阶（3rd-order）。

* 力生成机制决定了作为输入提供给执行器的标量控制信号如何映射为标量力，该标量力随后通过由传输推断出的**力臂（moment arms）**映射为广义力。

##### 传输 (Transmission)

传输 (Transmission)指定力要作用在哪里。

* Joint (关节型)：直接在关节上施加力矩（Torque）。
* Tendon (肌腱型)：像肌肉一样拉动绳索。
* Slider-Crank (曲柄滑块)：将直线运动转化为旋转运动。

##### 激活动力学 (Activation Dynamics)

激活动力学 (Activation Dynamics)表示系统的阶数

* 无激活状态 (0-order / Algebraic)：
  * 模型：$Force = Gain \cdot u$
  * 此时输入 $u$ 直接产生力。这是理想电机的模型（假设电流响应无限快）。
  * 系统阶数：2阶（位置 $q$，速度 $\dot{q}$）。

* 有激活状态 (1st-order dynamics)：
  * 模型:：$\dot{a} = \frac{1}{\tau}(u - a)$，然后 $Force = f(a)$。
  * 此时输入 $u$ 改变的是力的变化率，而不是力本身。比如液压缸，你打开阀门 ($u$)，油压 ($a$) 是逐渐建立的。
  * 系统阶数：3阶。状态向量变为 $[q, \dot{q}, a]$。

##### 力生成 (Force Generation)

力生成 (Force Generation)描述输入输出的关系，是增益（Gain）和非线性特性的定义层。

* 简单的：$F = k \cdot u$（线性电机）
* 复杂的：生物肌肉的力不仅取决于输入，还取决于当前的长度（Length-Tension relationship）和收缩速度（Velocity-Tension relationship）

#### Sensor

MuJoCo 可以生成模拟的传感器数据，这些数据保存在全局数组`mjData.sensordata`中。

这些结果不用于任何内部计算（物理引擎解算）；提供它们是因为用户可能需要将其用于自定义计算或数据分析。

可用的传感器类型包括：接触传感器（touch）、惯性测量单元 (IMUs)、力-扭矩传感器、关节和肌腱的位置及速度传感器、执行器的位置、速度和力传感器、动作捕捉标记点的位置和四元数，以及磁力计。

其中一些需要额外的计算（如模拟加速度计），而另一些则是直接从`mjData`的相应字段中复制的。

还有一个 `User Sensor`（用户传感器），允许用户代码将任何其他感兴趣的量插入到传感器数据数组中。

`MuJoCo`还具有**离屏渲染（off-screen rendering）**功能，这使得模拟彩色（RGB）和深度（Depth）相机传感器变得非常简单。

注意：相机数据不包含在标准的`sensordata`模型中，而是必须通过编程方式完成，如代码示例`simulate.cc`所示。

#### Equality Constraints

等式约束可以在运动学树结构及其定义的关节/自由度所施加的约束之外，施加额外的约束。可用于创建闭环关节（loop joints），或者通常用于模拟机械耦合。

强制执行这些约束的内力是与所有其他约束力一起计算的。

可用的等式约束类型包括：

* 点连接（Connect）：在某一点连接两个刚体（实际上是在运动学树之外创建一个球关节）；

* 焊接（Weld）：将两个刚体刚性地焊接在一起；

* 固定（Fix）：固定关节或肌腱的位置；

* 耦合（Couple）：通过三次多项式耦合两个关节或两个肌腱的位置；

* 柔性体约束：将柔性体（即变形网格）的边缘限制为其初始长度。

`MuJoCo`的等式约束默认是硬约束（`Hard Constraint`）。这意味着求解器会不惜一切代价（产生无限大的力）来保证 $pos_1 = pos_2$。

#### Flex

柔性体（Flexes） 代表了可变形的网格，它们可以是 1 维、2 维或 3 维的（因此它们的单元分别是胶囊体、三角形或四面体）。

与刚性附着在单个刚体上的静态形状 Geom 不同，Flex 的单元是可变形的：它们是通过连接多个刚体（bodies）构建的，因此刚体的位置和方向在运行时决定了 Flex 单元的形状。

这些可变形单元支持碰撞和接触力，以及产生被动力和约束力，以“柔和地”保持可变形实体的形状。

#### Contact Pair

`MuJoCo`中的接触生成是一个复杂的过程。

被检查是否发生接触的 Geom 对（Geom pairs） 来源有两个：

* 自动化的**邻近测试（proximity tests）**和其他统称为“动态（dynamic）”的过滤器；

* 以及模型中提供的显式 Geom 对列表。

可以用`Contact Pair`定制化摩擦与参数

```xml
<contact>
    <pair geom1="geom_foot" geom2="geom_floor" condim="3" friction="1.5 0.005 0.0001"/>
</contact>
```

#### Contact Exclude

这是接触对的反面：它指定了应该从候选接触对生成中排除的刚体对（Body pairs，注意不是 Geoms）。
