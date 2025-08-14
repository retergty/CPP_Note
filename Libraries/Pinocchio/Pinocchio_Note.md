# Pinocchio

`Pinocchio`是一个用于高效计算机器人模型或任何铰接刚体模型（模拟器中的化身、生物力学的骨骼模型等）的动力学（和导数）的库。

参考文档

* [Pinocchio](https://gepettoweb.laas.fr/doc/stack-of-tasks/pinocchio/devel/doxygen-html/)

## 安装步骤

* [Installation Procedure](https://stack-of-tasks.github.io/pinocchio/download.html)

## CMAKE配置

```CMake
find_package(pinocchio REQUIRED)
target_link_libraries(pinocchio_test PUBLIC pinocchio::pinocchio)
```

## 概念

### 几何学

刚体系统是由关节、刚体和力组成的装配体。关节连接两个不同的刚体，并收集它们之间的所有运动学关系，允许在两个刚体之间创建相对位移。这种位移通过分解为三个部分来描述：旋转、平移或旋转与平移的组合。

旋转矩阵构成了所谓的特殊正交群 $SO(n)$。其中有两个子群与我们当前的研究相关：$SO(2)$和 $SO(3)$。$SO(3)$是三维空间中所有旋转的集合，其元素为 $3×3$ 矩阵。$SO(2)$ 适用于平面问题，是二维空间中旋转的集合，其元素为 $2×2$ 矩阵。

所有齐次变换矩阵的集合构成特殊欧氏群 $SE(n)$。与旋转矩阵类似，这里也有两个不同的群：$SE(3)$用于三维变换，$SE(2)$用于二维变换（即平面内的变换）。

### ​​切空间与动力学

切空间是描述物体运动轨迹的向量空间，包含所有切向速度向量。通过切空间可简化轨迹计算，利用欧氏空间的叉乘和线性组合特性。例如，分析物体轨迹时，切空间中的速度向量可表示为：

$$
T_pM  = \{\gamma\prime(0) \;|\; \gamma: R \to M, \gamma(0) = p \}
$$

### 关节

李代数下，关节的表示为

* 旋转关节是一个 $SO(2)$ 对象,单自由度旋转

$$
Mat_{move} = \begin{bmatrix} 0\\0\\1\\0\\0\\0 \end{bmatrix} \ \ Mat_{cons} = \begin{bmatrix} 1 &0 &0 &0 &0 \\ 0 &1 &0 &0 &0 \\0 &0 &0 &0 &0 \\0 &0 &1 &0 &0 \\ 0 &0 &0 &1 &0 \\0 &0 &0 &0 &1 \end{bmatrix}
$$

* 圆柱关节（Cylindrical Joint）​,旋转+平移

$$
Mat_{move} = \begin{bmatrix} 0 &0 \\ 0 &0 \\ 1 &0 \\ 0 &0 \\ 0 &0 \\ 0 &1 \end{bmatrix} \ \ Mat_{const} = \begin{bmatrix} 1 &0 &0 &0 \\ 0 &1 &0 &0 \\ 0 &0 &0 &0 \\ 0 &0 &1 &0 \\ 0 &0 &0 &1 \\ 0 &0 &0 &0 \end{bmatrix}
$$

* 球形关节（Spherical Joint）​是 $SO(3)$ 对象,三维空间旋转

$$
Mat_{move} = \begin{bmatrix} 1 &0 &0\\0 &1 &0\\0 &0 &1\\0 &0 &0\\0 &0 &0\\0 &0 &0 \end{bmatrix} \ \ Mat_{cons} = \begin{bmatrix} 0 &0 &0 \\ 0 &0 &0 \\0 &0 &0 \\1 &0 &0 \\ 0 &1 &0 \\0 &0 &1 \end{bmatrix} 
$$

* 平面关节（Planar Joint）,平面平移+旋转

$$
Mat_{move} = \begin{bmatrix} 0 &0 &0\\0 &0 &0\\0 &0 &0\\1 &0 &0\\0 &1 &0\\0 &0 &1 \end{bmatrix} \ \ Mat_{cons} = \begin{bmatrix} 1 &0 &0 \\ 0 &1 &0 \\0 &0 &1 \\0 &0 &0 \\ 0 &0 &0 \\0 &0 &0 \end{bmatrix}
$$

* 自由浮动关节（Free Floating Joint）​,三维空间刚体运动

$$
Mat_{move} = \begin{bmatrix} Id \end{bmatrix} \ \ Mat_{const} = \begin{bmatrix} 0 \end{bmatrix}
$$
