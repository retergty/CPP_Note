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
* `matrix.topRows(q)`,`matrix.topRows<q>()`顶部的`q`行子块.
* `matrix.bottomRows(q)`,`matrix.bottomRows<q>()`底部的`q`行子块
* `matrix.leftCols(p)`,`matrix.leftCols<p>()`,左边的`p`列子块
* `matrix.rightCols(q)`,`matrix.rightCols<q>()`右边的`q`列子块
* `matrix.middleCols(i,q)`,`matrix.middleCols<q>(i)`中间的`q`列子块，从第`i`列开始.
* `matrix.middleRows(i,q)`,`matrix.middleRows<q>(i)`中间的`q`行子块，从第`i`行开始

### 用于向量的单维度的操作

* `vector.head(n)`,`vector.head<n>()`向量开头的`n`个元素.
* `vector.tail(n)`,`vector.tail<n>()`向量末尾的`n`个元素.
* `vector.segment(i,n)`,`vector.segment<n>(i)`向量从第`i`个元素开始的`n`个元素.

## 矩阵片段`slice`

矩阵`slice`比块操作更加灵活，可以获取任意行或列的组合.

参考文档

* [Slicing and Indexing](https://eigen.tuxfamily.org/dox/group__TutorialSlicingIndexing.html)

### `operator()`

`Eigen`的`operator()`函数可以接受以下几类的参数

* 整数，表示单独的行或列.
* `Eigen::all`代表所有的行或者所有的列，按照升序排列.
* `ArithmeticSequence`类，可以通过`Eigen::seq`,`Eigen::seqN`或`Eigen::placeholders::lastN`构建.
* 一维的`vector`或`array`,包括`Eigen::vector`,`std::array`,`std::vector`或者是标准的C风格的数组`int[N]`.

总而言之，`operator()`可以接受任何导出了如下成员函数的对象.

* `<integral type> operator[](<integral type>) const;`
* `<integral type> size() const;`

`integral type`是任何可以转换为`Eigen::Index`的类型.

### `Eigen::seq`

利用`Eigen::seq`或`Eigen::seqN`可以获取一组行或者列。

* `seq(firstIdx,lastIdx)`,表示从`firstIdx`到`lastIdx`的封闭整数序列，比如`seq(2,5) <=> {2,3,4,5}`
* `seq(firstIdx,lastIdx,incr)`,表示从`firstIdx`到`lastIdx`的封闭整数序列，但是步长为`incr`,比如`seq(2,8,2) <=> {2,4,6,8}`.
* `seqN(firstIdx,size)`,表示从`firstIdx`开始的`size`个整数序列,比如`seqN(2,5) <=> {2,3,4,5,6}`.
* `seqN(firstIdx,size,incr)`，表示从`firstIdx`开始的`size`个整数序列，步长为`incr`，比如`seqN(2,3,3) <=> {2,5,8}`

`Eigen::last`符号可以用来获取最后的行或者列.

* `A(seq(i,last), seqN(0,n))`,边界是第`i`行第`n`列的左下快.
* `A(all, seq(0,last,2))`,所有偶数列.
* `A(all, last-1)`,倒数第二列.
* `A(last/2,all)`中间的行.

指定最后`n`个元素可以使用`Eigen::placeholders::lastN(size)`,`Eigen::placeholders::lastN(size,incr)`.

* `v(lastN(n))`最后`n`个元素，等价于`v.tail(n)`.
* `A(all, lastN(n,3))`,最后`n`个元素，但是步长为`3`.

### 编译期指定size与incr

使用`Eigen::fix<val>`可以在编译期指定`size`与`incr`.

* `v(seq(last-fix<7>, last-fix<2>))`
* `A(all, seq(0,last,fix<2>))`编译期指定偶数列

### 反转行或列

只要指定`incr`为负数，那么便可以反转行或者列，原理就是生成一个递减的整数序列。

* `A(all, seq(20, 10, fix<-2>))`从第`20`列开始到第`10`列，步长为`-2`的矩阵.
* `A(seqN(last, n, fix<-1>), all)`从最后一行开始的后`n`行。

`ArithmeticSequence::reverse()`也可以用于反转整数序列.

### `array`

`operator()`也可以用来接受`std::array`等来指定任意的整数序列。

* `std::vector<int> ind{4,2,5,5,3};`,`A(Eigen::placeholders::all,ind)`表示`ind`指定的列拼成的矩阵.

## 特殊的矩阵或数组

参考文档

* [Special matrices and arrays](https://eigen.tuxfamily.org/dox/group__TutorialAdvancedInitialization.html)

有以下`static`函数，返回特殊的矩阵或者数组

* `Matrix::Zero()`返回零矩阵
* `MatrixXd::Constant(rows, cols, value)`，返回所有元素设置为`value`的矩阵
* `MatrixXd::Random()`,返回随机矩阵
* `Matrix::Identity()`，返回`I`阵.
* `LinSpaced(size, low, high)`返回`[low,high]`的线性插值，个数为`size`，只能用于一维。

除此以外，还有`setZero`等成员函数，把对象设置为这些矩阵。