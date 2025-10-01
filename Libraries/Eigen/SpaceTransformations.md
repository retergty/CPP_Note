# 空间变换

参考文档

* [Space transformations](https://eigen.tuxfamily.org/dox/group__TutorialGeometry.html)

定义在`<Eigen/Geometry>`里

`Eigen`提供了特殊的类来解决二维或三维的旋转，投影以及仿射变换。

* 抽象的变换，比如轴角，四元数等，不是矩阵，不能和矩阵混用.
* 投影和仿射变换使用矩阵来表示。

## 空间变换类

| 变换类型 | 描述 |
|:-:|:-:|
| 角度表示的二维旋转  | `Rotation2D<float> rot2(angle_in_radian);`  |
|  轴角法表示的三维旋转 | `AngleAxis<float> aa(angle_in_radian, Vector3f(ax,ay,az));`  |
| 四元数表示的三维旋转  | `Quaternion<float> q;` <br> `q = AngleAxis<float>(angle_in_radian, axis);`  |
|  N维缩放 | `Scaling(sx, sy)` <br> `Scaling(sx, sy, sz)` <br> `Scaling(s)` <br> `Scaling(vecN)`|
|  N维移动 | `Translation<float,2>(tx, ty)` <br> `Translation<float,3>(tx, ty, tz)` <br> `Translation<float,N>(s)` <br> `Translation<float,N>(vecN)`  |
|  N维仿射变换 | `Transform<float,N,Affine> t = concatenation_of_any_transformations;` <br> `Transform<float,3,Affine> t = Translation3f(p) * AngleAxisf(a,axis) * Scaling(s);`  |
|  N维线性变换 | `Matrix<float,N> t = concatenation_of_rotations_and_scalings;` <br> `Matrix<float,2> t = Rotation2Df(a) * Scaling(s);` <br> `Matrix<float,3> t = AngleAxisf(a,axis) * Scaling(s);` |

以上的空间变换类都可以自由地从其它类型中构造.

## 通用API

`Eigen`提供了通用的空间变换API

* `gen1 * gen2;`两个变换的连接
* `vec2 = gen1 * vec1;`对向量应用变换
* `gen2 = gen1.inverse();`变换的逆变换
* `rot3 = rot1.slerp(alpha,rot2);`球面插值,(只能用在二维与四元数中)

## Transform类

```CPP
template<typename Scalar_, int Dim_, int Mode_, int Options_>
class Eigen::Transform< Scalar_, Dim_, Mode_, Options_ >
```

模板参数

* `Scalar_`元素使用的存储类型
* `Dim_`空间的维数
* `Mode_`模式，可以是
  * `Affine`,`Transform`类存储在`(Dim+1)^2`矩阵中，同时最后一行认为是`[0 ... 0 1]`
  * `AffineCompact`,`Transform`类存储在`(Dim)x(Dim+1)`矩阵中
  * `Projective`,`Transform`类存储在`(Dim+1)^2`矩阵中，不做任何假设
  * `Isometry`,和`Affine`但是认为线性部分表示旋转，可以用于加快某些函数的计算，注意，这只是假设，实际线性部分还可能包含缩放，这个取决于用户.
* `Options_`和`Matrix`里的`Options_`一样

`Transform`类理论上的格式为

$$
\begin{pmatrix}
linear & Translation \\
0...0 & 1
\end{pmatrix}
$$

平移部分为

$$
\begin{pmatrix}
I & Translation \\
0...0 & 1
\end{pmatrix}
$$

旋转部分为

$$
\begin{pmatrix}
R & 0 \\
0...0 & 1
\end{pmatrix}
$$

缩放部分为

$$
\begin{pmatrix}
S & 0 \\
0...0 & 1
\end{pmatrix}
$$

### 构造

`Transform`类可以通过任何空间变换类进行构建.

```CPP
Transform t;
t = AngleAxis(angle,axis);
```

```CPP
Transform<float,3,Affine> t = Translation3f(p) * AngleAxisf(a,axis) * Scaling(s);
```

## AngleAxis类

```CPP
template<typename Scalar_>
class Eigen::AngleAxis< Scalar_ >
```

表示绕着指定三维向量(必须标准化)旋转指定角度的旋转.

```CPP
Matrix3f m;
m = AngleAxisf(0.25*M_PI, Vector3f::UnitX())
  * AngleAxisf(0.5*M_PI,  Vector3f::UnitY())
  * AngleAxisf(0.33*M_PI, Vector3f::UnitZ());
```

使用这个可以很方便地表示旋转的组合，以及欧拉角.

比如这个例子就表示了`ZYX`转序.

### angle

```CPP
Scalar & angle ()

Scalar angle () const
```

获取角度，角度范围在`[0,PI]`范围

### axis

```CPP
Vector3 & axis ()
const Vector3 & axis () const
```

获取旋转轴.

## Quaternion类

```CPP
template<typename Scalar_, int Options_>
class Eigen::Quaternion< Scalar_, Options_ >
```

四元数类可以很方便地表示旋转.

### 构造函数

```CPP
Quaternion (const Scalar &w, const Eigen::MatrixBase< Derived > &vec)

Quaternion (const Scalar &w, const Scalar &x, const Scalar &y, const Scalar &z)
 
Quaternion (const Scalar *data)
 
Quaternion (Quaternion &&other)
```
