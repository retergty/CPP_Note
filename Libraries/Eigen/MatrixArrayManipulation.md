# 基础矩阵操作

## 矩阵Matrix

参考文档

* [The Matrix class](https://eigen.tuxfamily.org/dox/group__TutorialMatrixClass.html)

```CPP
Matrix<typename Scalar,
       int RowsAtCompileTime,
       int ColsAtCompileTime,
       int Options = 0,
       int MaxRowsAtCompileTime = RowsAtCompileTime,
       int MaxColsAtCompileTime = ColsAtCompileTime>
```

`Eigen`矩阵类型，默认在内存中以按列存储。

* `Scalar`表示矩阵所存储的数据类型.
* `RowsAtCompileTime`,`ColsAtCompileTime`表示编译期可知的矩阵行数与列数，可以是特殊值`Dynamic`，表示运行时可以改变矩阵的行数或列数.
* `Options`可选的比特位域，比如`RowMajor`,表示按行存储。
* `MaxRowsAtCompileTime`,`MaxColsAtCompileTime`表示最大允许的矩阵行列数，哪怕之前的参数是`Dynamic`，最大的用处是避免了动态内存分配，比如`Matrix<float, Dynamic, Dynamic, 0, 3, 4>`

* [matrix](https://eigen.tuxfamily.org/dox/classEigen_1_1Matrix.html)

### 向量Vector

```CPP
typedef Matrix<float, 3, 1> Vector3f;
```

向量就是列为`1`的矩阵.

### 构造函数

```CPP
template<typename Scalar_ , int Rows_, int Cols_, int Options_, int MaxRows_, int MaxCols_>
Eigen::Matrix< Scalar_, Rows_, Cols_, Options_, MaxRows_, MaxCols_ >::Matrix()
```

  对于固定长度的矩阵，什么都不做

  对于动态长度的矩阵，创建一个维数为0的矩阵，不会分配任何空间.

```CPP
template<typename Scalar_ , int Rows_, int Cols_, int Options_, int MaxRows_, int MaxCols_>
Eigen::Matrix< Scalar_, Rows_, Cols_, Options_, MaxRows_, MaxCols_ >::Matrix(const std::initializer_list< std::initializer_list< Scalar >> &list)
```

  可以使用`initializer_list`初始化矩阵.例如

  ```CPP
  MatrixXi a {      // construct a 2x2 matrix
      {1, 2},     // first row
      {3, 4}      // second row
  };
  Matrix<double, 2, 3> b {
        {2, 3, 4},
        {5, 6, 7},
  };
  ```

  注意是按行组织起来的.

```CPP
Matrix3f m;
m << 1, 2, 3,
     4, 5, 6,
     7, 8, 9;
std::cout << m;
```

  可以使用逗号表达式初始化矩阵.这也是按行组织的.

### 访问元素

使用`operator()`运算符就可以访问元素，下标从零开始.

### 修改矩阵维数

```CPP
template<typename Derived >
void Eigen::PlainObjectBase< Derived >::resize(Index rows,Index cols)
```

  把动态矩阵的维数修改为`rows,cols`.如果维数确实发生了改变，那么会重新分配空间，并销毁之前分配的空间.

  如果想要尽量保存之前矩阵的值，可以使用`conservativeResize()`

  如果只想修改行数或者是列数，可以使用`resize`的重载版本.

  ```CPP
  m.resize(NoChange, 5);
  ```

```CPP
MatrixXf a(2,2);
MatrixXf b(3,3);
a = b;
```

  对于动态矩阵，`operator=`会把左边的矩阵维数与值修改成右边矩阵。

## 矩阵运算

参考文档

* [Matrix and vector arithmetic](https://eigen.tuxfamily.org/dox/group__TutorialMatrixArithmetic.html)

常见的算术运算符都具有显然的矩阵运算意义

### 转置，共轭，共轭转置

* `a.transpose()`表示矩阵转置
* `a.conjugate()`表示矩阵共轭
* `a.adjoint()`表示矩阵共轭转置

对于实数矩阵，矩阵共轭没有任何作用

注意防止混叠问题，矩阵就地转置需要使用

* `a.transposeInPlace()`表示矩阵就地转置
* `a.adjointInPlace()`表示矩阵就地共轭转置

### 点乘，叉乘

* `v.dot(w)`表示向量点乘，或者是向量内积
* `v.cross(w)`表示向量叉乘

### 基本算术归约运算

* `mat.sum()`表示所有元素的和
* `mat.prod()`表示所有元素的乘积
* `mat.mean()`表示元素平均值
* `mat.minCoeff()`表示元素最小值
* `mat.maxCoeff()`表示元素最大值
* `mat.trace()`表示矩阵的迹

## 数组Array

参考文档

* [The Array class and coefficient-wise operations](https://eigen.tuxfamily.org/dox/group__TutorialArrayClass.html)

数组Array提供通常意义上的数组。矩阵Matrix的运算具有线性代数的意义，数组的运算则是执行元素间的简单运算.（coefficient-wise）

```CPP
Array<typename Scalar,
       int RowsAtCompileTime,
       int ColsAtCompileTime,
       int Options = 0,
       int MaxRowsAtCompileTime = RowsAtCompileTime,
       int MaxColsAtCompileTime = ColsAtCompileTime>
```

和矩阵一样的模板参数声明.构造方法与访问方法和矩阵无异.

### 加减

* 相同的Array加减同矩阵加减一样
* `Array+Scalar`类型表示给`Array`每个元素都加上`Scalar`.

### 数组乘

* `Array*Scalar`和矩阵数乘相同.
* `Array*Array`表示数组每个元素相乘，结果是相同长度的数组.

### 其它的运算

* `a.abs()`数组每个元素的绝对值
* `a.sqrt()`数组每个元素的平方
* `a.min(b)`返回一个数组，这个数组对应元素为`a,b`中对应位置元素的小者.

### 数组与矩阵的转换

* `m.array()`把矩阵转换为数组
* `a.matrix()`把数组转换为矩阵

利用`Eigen`的表达式模板系统，转换发生在编译期。且都可以作为左值和右值.

```CPP
m.array() * n.array();
m.cwiseProduct(n);
```

## 块操作

参考文档

* [Block operations](https://eigen.tuxfamily.org/dox/group__TutorialBlockOperations.html)

块`block`是矩阵或者数组的矩形子部分.可以作为左值与右值使用.

```CPP
matrix.block(i,j,p,q);
matrix.block<p,q>(i,j);
```

表示矩阵`m`从`i`行,`j`列开始的`p`行`q`列子块.

```CPP
a.block<2,2>(1,1) = m;
```

块可以作为左值，向底层的矩阵写入特定长度的块.

### 特定行或列

* `matrix.row(i)`表示矩阵第`i`行
* `matrix.col(j)`表示矩阵第`j`列

### 有关边界的操作

* `matrix.topLeftCorner(p,q)`,`matrix.topLeftCorner<p,q>()`,从左上角开始的`p`行`q`列子块.
* `matrix.bottomLeftCorner(p,q)`,`matrix.bottomLeftCorner<p,q>()`,从左下角开始的`p`行`q`列子块.
* `matrix.topRightCorner(p,q)`,`matrix.topRightCorner<p,q>()`，从右上角开始的`p`行`q`列子块.
* `matrix.bottomRightCorner(p,q)`,`matrix.bottomRightCorner<p,q>()`，从右下角开始的`p`行`q`列子块.
* `