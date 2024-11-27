# Eigen

本文介绍`Eigen`库，这是最常见的`C++`线性代数库，实现了许多线性代数的功能。

参考文档

* [Eigen官方文档](https://eigen.tuxfamily.org/dox/index.html)

`Eigen`库的功能分为以下四类

* 基础的矩阵运算.
* 线性方程组的求解与矩阵分解
* 稀疏矩阵算法
* 空间变换问题

## 矩阵Matrix

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