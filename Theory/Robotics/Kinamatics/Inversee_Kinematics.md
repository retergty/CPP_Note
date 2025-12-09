# 逆运动学

本文总结逆运动学的数值解法.

末端执行器需要移动到空间中的对应位置,并且处于一定的姿态中.也就是，需要控制末端执行器所在的坐标系的位置与姿态.

$$
\begin{pmatrix}
p^w_{wn} \\
{^w_nR}
\end{pmatrix}
$$

## 问题描述

对于期望的末端执行器坐标系`n`的位置与姿态.

$$
\begin{pmatrix}
p^w_{wn} \\
{^w_nR}
\end{pmatrix}
$$

由于旋转矩阵不是加法群，将旋转矩阵变为欧拉角.

$$
y_d =
\begin{pmatrix}
p^w_{d} \\
\alpha_d \\
\beta_d \\
\gamma_d
\end{pmatrix}
$$

求解$\theta$,使得

$$
min（||(y_d-f(\theta))||_2)
$$

其中

$$
\begin{pmatrix}
p^w_{wn} \\
\alpha \\
\beta \\
\gamma
\end{pmatrix} =
f(\theta)
$$

是关节的正运动学.

## Levenberg–Marquardt方法

对于任意的向量函数

$$
y = f(x)
$$

对于向量$y_d$,最小化目标函数

$$
\min_x F(x) = \frac12(y_d - f(x))^T(y_d - f(x))
$$

进行泰勒展开

$$
y = y_k + \frac{\partial{f}}{\partial{x}}(dx)
$$

设

$$
J = \frac{\partial{f}}{\partial{x}}
$$

使用一阶线性化估计，有

$$
\delta y = y_d - y_k = J \cdot dx
$$

计算一阶线性子问题，使得

$$
\min_{dx} G(dx) = \frac12(y_d - f(x_k)-Jdx)^T(y_d - f(x_k)-Jdx)
$$

为防止$dx$过大，加入$dx$优化项.

$$
\min_{dx}E(dx) = \frac12(\delta y - J \cdot dx)^T(\delta y - J \cdot dx) + \frac\lambda2(dx)^Tdiag(J^TJ)(dx)
$$

其中，$\lambda$为阻尼值，越大则越快速，越小则越精确.$J^TJ$是加权数，保证$dx$的各个分量均一的量纲.

这是标准的二次规划的问题.

求偏导数

$$
\frac{\partial E(dx)}{\partial dx} = 0
$$

解得

$$
dx = (J^TJ + \lambda diag(J^TJ))^{-1}J^T\delta y
$$

进行迭代

$$
x_{k+1} = x_k + \alpha dx
$$

$\alpha$为迭代步长系数.

重复计算，直到达到误差容限.

### $\lambda$的选择方法1

$\lambda$直接影响求解的速度与精度，但$\lambda$的选择是经验性的,可以这样选择.

1. 计算初始值$\lambda_0=\tau \cdot max(diag(J^TJ))$对角线元素最大值,对于非病态问题，可设为$\lambda_0 = 10^{-3} \sim 10^{-2}$
2. 从初始值$\lambda = \lambda_0$开始，选择一个系数$\nu > 1$.
3. 分别以$\lambda,\ \lambda/\nu$,计算迭代误差平方和.$(y_d-f(x_0+dx))^2$
4. 如果误差都增大，那么反复加大$\lambda$,$\lambda = \lambda \nu$,重新计算误差平方和，直到下降情况.
5. 如果$\lambda/\nu$导致了误差平方和下降，那么就以$\lambda / \nu$作为新的$\lambda$,继续迭代.注意，不能下降得过多.需要有一个最小值.
6. 如果$\lambda$导致误差平方和下降($\lambda / \nu$升高)，那么就保持$\lambda$继续迭代.

### $\lambda$的选择方法2

方法2使用误差变化率来选择$\lambda$

定义误差变化率

$$
\begin{align*}
\rho &= \frac{F(x_k)-F(x_k+dx)}{|G(x_k) - G(x_k+dx)|} \\
&= \frac{(y_d - f(x_k))^T(y_d - f(x_k)) - (y_d - f(x_k + dx))^T(y_d - f(x_k + dx))}{|dx^T(\lambda diag(J^TJ) dx + J^T(y_d-f(x_k)))|}
\end{align*}
$$

1. 计算初始值$\lambda_0=\tau \cdot max(diag(J^TJ))$对角线元素最大值,对于非病态问题，可设为$\lambda_0 = 10^{-3} \sim 10^{-2}$
2. 从初始值$\lambda = \lambda_0$开始，选择一个系数$\nu > 1$.
3. 如果$\rho < \epsilon_0$,则误差下降率较慢，需要$\lambda = \lambda v$,重新计算.
4. 如果$\rho < \epsilon_1$,则误差下降率一般，保持$\lambda$,接受$dx$.
5. 否则，误差下降率较快，减小$\lambda = \lambda /v$,接受$dx$.
6. 重复直到$F(x)$满足需求.

通常可取，$\epsilon_0 = 0.25,\epsilon_1 = 0.75$.

## 逆运动学数值解法

使用`Levenberg–Marquardt`方法,求解逆运动学数值解.

$$
\begin{pmatrix}
p^w_{wn} \\
\alpha \\
\beta \\
\gamma
\end{pmatrix} =
f(\theta) \\
\frac{\partial{f}}{\partial{\theta}} = \begin{pmatrix}
a^w_1 \times (p^w_n - p^w_1) & a^w_2 \times (p^w_n - p^w_2) & ... & a^w_{n-1} \times (p^w_n - p^w_{n-1}) & 0 \\
a^w_1 & a^w_2 & ... & a^w_{n-1} & a^w_n
\end{pmatrix} = J
$$

每一步均需要计算出雅可比矩阵，使用这个来进行迭代.
