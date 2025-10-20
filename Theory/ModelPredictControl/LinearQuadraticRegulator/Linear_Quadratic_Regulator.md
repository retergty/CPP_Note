# Linear Quadratic Regulator

参考文档

* [LQR](https://zhuanlan.zhihu.com/p/715102938)

LQR全称为Linear Quadratic Regulator线性二次型调节器，求解线性系统最优控制的方法

## 最优控制问题

$$
\begin{align*}
\min_{u_0,...u_{N-1}} J &= \frac{1}{2}x^T_NHx^T_N + \frac{1}{2}\sum^{N-1}_{k=0}[x^T_kQ_kx_k + u^T_kR_ku_k] \\
{\mathbb{s.t.}} \quad x_{k+1} &= Ax_k + Bu_k \\
H & \in S^n_+ \\
Q & \in S^n_+ \\
R & \in S^n_+
\end{align*}
$$

## 算法原理

利用贝尔曼最优性原理，该原理指出，一个状态的最优值是最小化的“到达成本”与后续状态值$\bf{x}^\prime$的组合

$$
V^*({\bf{x}}) = \underset{{\bf{u}}}{min}[g({\bf{x}},{\bf{u}}) + V^\ast(\bf{x}^\prime)]
$$

代入上文的符号，可得

$$
\hat J_i(x_i) = \min_{u_i}\{\frac{1}{2}x^T_iQ_ix_i + \frac{1}{2}u^T_iR_iu_i + \hat J_{i+1}(x_{i+1})\}
$$

其中，$\hat J_i(x_i)$是在状态$x_i$时，最小的到达成本。注意$\hat J_i$和$\hat J_{i+1}$不是同一个函数.

已知最终状态

$$
\begin{align*}
\hat J_N(x_N) &= \frac{1}{2}x^T_NHx^T_N \\
&= \frac{1}{2}(Ax_{N-1}+Bu_{N-1})^TH(Ax_{N-1}+Bu_{N-1})
\end{align*}
$$

带入上式可得

$$
\begin{align*}
\hat J_{N-1}(x_{N-1}) &= \min_{u_{N-1}}\{u^T_{N-1}(R_{N-1} + B^THB)u_{N-1} + 2x^T_{N-1}A^THBu_{N-1} + x^T_{N-1}(Q_{N-1} + A^THA)x_{N-1} \} \\
&= \tilde J_{N-1}(x_{N-1},u_{N-1})
\end{align*}
$$

反向推导$\hat J_{N-1}(x_{N-1})$，对$u_{N-1}$求偏导，并令其等于0有

$$
\frac{\partial \tilde J}{\partial u_{N-1}} =
\hat J_{N-1}(x_{N-1}) = u^T_{N-1}(R_{N-1} + B^THB) + x^T_{N-1}A^THB = 0
$$

于是，在$N-1$时刻的最优控制输入为

$$
\begin{align*}
u^\ast_{N-1} &= -(R_{N-1} + B^THB)B^THAx_{N-1} \\
&= -K_{N-1}x_{N-1}
\end{align*}
$$

将最优控制输入代回前式有.

$$
\hat J_{N-1}(x_{N-1}) = x_{N-1}^T(Q_{N-1} + A^THA - A^THB(R + B^THB)^{-1}B^THA)x_{N-1}
$$

设

$$
P_{N} = H \\
P_{N-1} = Q_{N-1} + A^TP_{N}A - A^TP_{N}B(R + B^TP_{N}B)^{-1}B^TP_{N}A
$$

得到

$$
\hat J_{N-1}(x_{N-1}) = x_{N-1}^TP_{N-1}x_{N-1}
$$

可见结构与$N-1$时相同，所以得到了矩阵$P$的递推关系式

$$
P_i =  Q_{i} + A^TP_{i+1}A - A^TP_{i+1}B(R + B^TP_{i+1}B)^{-1}B^TP_{i+1}A
$$

称为离散时间代数黎卡提方程(Discretetime Algebraic Riccati Equation).

## 算法流程

### 计算反馈增益(Backward Pass)

* 初始化$P_N = H$
* ${\textit{for}} \quad i = N-1,...,0$
  * 迭代$P_i \leftarrow P_{i+1}$ : $P_i = Q_i + A^TP_{i+1}A - A^TP_{i+1}B(R_i+B^TP_{i+1}B)^{-1}B^TP_{i+1}A$
  * 计算最优反馈增益$K_i=(R_i + B^TP_{i+1}B)^{-1}B^TP_{i+1}A$

### 更新控制输入和期望状态(Forward Pass)

* 给定初始条件$x_0$
* ${\textit{for}} \quad i=0,...,N-1$
  * 计算控制输入$u_i^\ast = -K_ix_i$
  * 更新状态$x_{i+1} = Ax_i + Bu^\ast_i$
