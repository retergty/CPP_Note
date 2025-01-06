# 线性代数与矩阵分解

参考文档

* [Linear algebra and decompositions](https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html)

## 线性方程求解

$$
Ax=b
$$

其中$A$,$b$都是矩阵，要求解对应的$x$.如果无解，获取最小二乘解，如果有多解，求取最小范数解等.

```CPP
#include <iostream>
#include <Eigen/Dense>
 
int main()
{
   Eigen::Matrix3f A;
   Eigen::Vector3f b;
   A << 1,2,3,  4,5,6,  7,8,10;
   b << 3, 3, 4;
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   std::cout << "Here is the vector b:\n" << b << std::endl;
   Eigen::Vector3f x = A.colPivHouseholderQr().solve(b);
   std::cout << "The solution is:\n" << x << std::endl;
}
```

列主元QR分解`colPivHouseholderQr`是一种矩阵分解，提供了`solve`成员函数.

## 最小二乘解

参考文档

* [Solving linear least squares systems](https://eigen.tuxfamily.org/dox/group__LeastSquares.html)

### 使用奇异值分解

最常用的最小二乘求解的矩阵分解法是奇异值分解`SVD decomposition`.`Eigen`提供了两类奇异值分解法，推荐使用的是`BDCSVD`,适用于高维矩阵，同时会自动对于小矩阵退化为`JacobiSVD`.

```CPP
#include <iostream>
#include <Eigen/Dense>
参考文档

* [Linear algebra and decompositions](https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html)
 
int main()
{
   Eigen::MatrixXf A = Eigen::MatrixXf::Random(3, 2);
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   Eigen::VectorXf b = Eigen::VectorXf::Random(3);
   std::cout << "Here is the right hand side b:\n" << b << std::endl;
   std::cout << "The least-squares solution is:\n"
        << A.template bdcSvd<Eigen::ComputeThinU | Eigen::ComputeThinV>().solve(b) << std::endl;
}
```

同时还可以使用完全正交分解`CompleteOrthogonalDecomposition`.

### 使用QR分解

三种QR分解可以用于求解最小二乘,`HouseholderQR`,`ColPivHouseholderQR`,`ColPivHouseholderQR`.

```CPP
MatrixXf A = MatrixXf::Random(3, 2);
VectorXf b = VectorXf::Random(3);
cout << "The solution using the QR decomposition is:\n"
     << A.colPivHouseholderQr().solve(b) << endl;
```

### 使用通常解

不相容方程组$Ax=b$的最小二乘解就是相容方程组$A^TAx=A^Tb$的最小范数解.

```CPP
MatrixXf A = MatrixXf::Random(3, 2);
VectorXf b = VectorXf::Random(3);
cout << "The solution using normal equations is:\n"
     << (A.transpose() * A).ldlt().solve(A.transpose() * b) << endl;
```

这是最快的解法，但是会损失精度.

## 特征值与特征向量

如果矩阵是`Hermite`矩阵，也就是

$$
A^H=A
$$

可以使用`SelfAdjointEigenSolver`，否则需要使用`EigenSolver`,`ComplexEigenSolver`.

```CPP
#include <iostream>
#include <Eigen/Dense>
 
int main()
{
   Eigen::Matrix2f A;
   A << 1, 2, 2, 3;
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> eigensolver(A);
   if (eigensolver.info() != Eigen::Success) abort();
   std::cout << "The eigenvalues of A are:\n" << eigensolver.eigenvalues() << std::endl;
   std::cout << "Here's a matrix whose columns are eigenvectors of A \n"
        << "corresponding to these eigenvalues:\n"
        << eigensolver.eigenvectors() << std::endl;
}
```

## 矩阵逆与行列式

计算矩阵逆与行列式通常在线性方程组中不是必要的.

许多矩阵分解的类都提供了`inverse()`与`determinant()`方法.也可以直接在`matrix`上使用`inverse()`与`determinant()`,但是只适合用在不超过四维的矩阵.

```CPP
#include <iostream>
#include <Eigen/Dense>
 
int main()
{
   Eigen::Matrix3f A;
   A << 1, 2, 1,
        2, 1, 0,
        -1, 1, 2;
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   std::cout << "The determinant of A is " << A.determinant() << std::endl;
   std::cout << "The inverse of A is:\n" << A.inverse() << std::endl;
}
```

## 将计算与构建分离开

所有的矩阵分解类都有默认构造函数，同时提供了`compute`方法,可以计算指定矩阵的分解.

```CPP
#include <iostream>
#include <Eigen/Dense>
 
int main()
{
   Eigen::Matrix2f A, b;
   Eigen::LLT<Eigen::Matrix2f> llt;
   A << 2, -1, -1, 3;
   b << 1, 2, 3, 1;
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   std::cout << "Here is the right hand side b:\n" << b << std::endl;
   std::cout << "Computing LLT decomposition..." << std::endl;
   llt.compute(A);
   std::cout << "The solution is:\n" << llt.solve(b) << std::endl;
   A(1,1)++;
   std::cout << "The matrix A is now:\n" << A << std::endl;
   std::cout << "Computing LLT decomposition..." << std::endl;
   llt.compute(A);
   std::cout << "The solution is now:\n" << llt.solve(b) << std::endl;
}
```

此外，还可以告诉构造函数即将进行分解的矩阵的维数，以便避免动态内存分配.

```CPP
HouseholderQR<MatrixXf> qr(50,50);
MatrixXf A = MatrixXf::Random(50,50);
qr.compute(A); // no dynamic memory allocation
```

## 矩阵秩

特定的矩阵分解方法提供`rank`成员函数获取秩,`isInvertible()`成员函数获取是否可逆，以及计算矩阵核空间与零空间的方法.

```CPP
#include <iostream>
#include <Eigen/Dense>
 
int main()
{
   Eigen::Matrix3f A;
   A << 1, 2, 5,
        2, 1, 4,
        3, 0, 3;
   std::cout << "Here is the matrix A:\n" << A << std::endl;
   Eigen::FullPivLU<Eigen::Matrix3f> lu_decomp(A);
   std::cout << "The rank of A is " << lu_decomp.rank() << std::endl;
   std::cout << "Here is a matrix whose columns form a basis of the null-space of A:\n"
        << lu_decomp.kernel() << std::endl;
   std::cout << "Here is a matrix whose columns form a basis of the column-space of A:\n"
        << lu_decomp.image(A) << std::endl; // yes, have to pass the original A
}
```


可以通过`setThreshold`设置阈值.

## 矩阵分解

`Eigen`支持多种形式的矩阵分解.不同的矩阵分解对矩阵的需求与精度不同.

参考文档

* [Catalogue of dense decompositions](https://eigen.tuxfamily.org/dox/group__TopicLinearAlgebraDecompositions.html)

文档总结了所有`Eigen`支持的矩阵分解与奇异值分解方法类.



