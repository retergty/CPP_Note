# 强化学习

本文总结强化学习常用公式，常用算法。

## 马尔可夫决策过程

马尔可夫决策(`Markov Decision Process`, MDP)是一种用于描述序贯决策问题的数学框架，其核心特点是：系统的未来状态仅依赖于当前状态和当前选择的动作（即满足“马尔可夫性”或“无后效性”），而与历史状态无关。

给定当前状态 $S_t$, 未来的状态 $S_{t+1}$ 和奖励 $R_{t+1}$ 仅依赖于当前状态和当前动作 $A_t$, 与之前状态 $(S_0,S_1,...,S_{t-1})$ 和动作无关 $(A_0,A_1,...,A_{t-1})$,数学表示为

$$
P(S_{t+1} = s^\prime, R_{t+1} = r \mid S_0,A_0,...,S_t,A_t) = P(S_{t+1} = s^\prime, R_{t+1} = r \mid S_t,A_t)
$$

一个完整的MDP通常由以下五元组 $(S,A,P,R,\gamma)$ 定义

* 状态空间 $S$: 所有可能状态的集合
* 动作空间 $A$: 智能体在每个状态下可选的动作集合
* 状态转移概率 $P$：给定当前状态 $s$ 和动作 $a$, 转移到下一个状态 $s^\prime$ 的概率即 $P(s^\prime \mid s , a)$
* 奖励函数 $R$: 执行动作 $a$ 后从状态 $s$ 转移到 $s^\prime$ 时获得的即时奖励，通常表示为 $R(s,a,s^\prime)$
* 折扣因子 $\gamma$：介于$[0,1]$之间的参数,用于权衡即时奖励与未来奖励的重要性.

### 策略$\pi$

策略(Policy)$\pi$，即从状态到动作的映射，如 $\pi(a \mid s)$ 表示状态 $s$ 下选择 $a$的概率

### 累计奖励$G$

定义为从时刻 $t$ 开始的未来奖励加权和

$$
G_t = r_t + \gamma r_{t+1} + \gamma^2 r_{t+2} + ... = \sum_{k=0}^\infty \gamma^k r_{t+k}
$$

### 马尔科夫决策中的轨迹

给定一个策略 $\pi$ 与一个有限时间步长 $T$.

轨迹便是

$$
\tau = (s_0,a_0,s_1,a_1,...,s_T,a_T)
$$

轨迹的可能性

$$
P(\tau \mid \pi) = P(s_0,a_0,...,s_T,a_T \mid \pi)  = P(S_0 = s_0,A_0 = a_0,...,S_T =s_T,A_T = a_T \mid \pi)
$$

根据概率的乘法公式,与马尔科夫性质可以得到

$$
\begin{align*}
P(\tau \mid \pi) &= p(s_0)\pi(a_0 \mid s_0)p(s_1 \mid s_0, a_0)\pi(a_1 \mid s_1)p(s_2 \mid s_1,a_1) \\
&= p(s_0) \prod_{k=0}^{T-1}\pi(a_k \mid s_k)p(s_{k+1} \mid s_k,a_k)
\end{align*}
$$

可以得到，一条初始状态已知的轨迹，概率分布完全由策略与转移概率有关

### 定义

* $a \sim \pi(\cdot \mid s)  \Leftrightarrow a \sim \pi$，表示动作 $a$ 是在策略 $\pi$下进行采样，或当前动作 $a$ 的分布是 $\pi$
* $s^\prime \sim p(\cdot \mid s,a) \Leftrightarrow s^\prime \sim p$,表示下一个状态 $s^\prime$的分布是 $p$
* $\tau \sim \pi$ 意味着 $\tau \sim P(\tau \mid \pi)$,表示轨迹 $\tau$ 在策略 $\pi$ 下的采样.
* ${\mathbb{E}}_{\tau \sim \pi}(Y(\tau))$, $Y$ 是 $\tau$ 的函数，表示轨迹在策略下的条件期望. $\sum_\tau Pr(\tau \mid \pi)Y(\tau)$
* ${\mathbb{E}}_{\tau \sim \pi}(Y(\tau) \mid S_0 = s) = \sum_\tau Pr(\tau \mid \pi,S_0 = s)Y(\tau)$
* ${\mathbb{E}}_{\tau \sim \pi}(Y(\tau)) = {\mathbb{E}}_{s_0 \sim p_0}({\mathbb{E}}_{\tau \sim \pi}(Y(\tau \mid S_0 = s)))$,全期望公式

### MDP问题

最大化累积奖励$R(\tau)$

$$
\max_\tau {\mathbb{E}}_{\tau \sim \pi}[R(\tau)]
$$

### 值函数Value function

* 同策略价值函数On-policy value function

$$
V_\pi(s) \triangleq {\mathbb{E}}_{\tau \sim \pi}(R(\tau) \mid S_0 = s)
$$

* 同策略动作价值函数On-policy action value function

$$
Q_\pi(s,a) \triangleq {\mathbb{E}}_{\tau \sim \pi}[R(\tau) \mid S_0 = s,A_0 = a]
$$

* 最优值函数Optimal Value function

$$
V^\ast(s) = \max_\pi(V_\pi(s)) = \max_\pi({\mathbb{E}}_{\tau \sim \pi}[R(\tau) \mid S_0 = s])
$$

* 最优动作价值函数Optimal action value function

$$
Q^\ast(s,a) = \max_\pi Q_\pi(s,a) =\max_\pi {\mathbb{E}}_{\tau \sim \pi}[R(\tau) \mid S_0 = s,A_0 = a]
$$

### 贝尔曼方程Bellman Equations

值函数推导过程为

$$
\begin{align*}
V_\pi(s) &\triangleq {\mathbb{E}}_{\tau \sim \pi}[R(\tau) \mid S_0 = s] \\
&= {\mathbb{E}}_{\tau \sim \pi}[r(S_0,A_0) + \alpha r(S_1,A_1) + \alpha^2 r(S_2,A_2) + ... \mid S_0 = s] \\
&= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}({\mathbb{E}}_{\tau \sim \pi} [r(S_0,A_0) + \alpha r(S_1,A_1) + ... \mid S_0 = s,A_0 = a]) \\
&= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}(r(s_0,a_0) + \alpha{\mathbb{E}}_{\tau \sim \pi} [r(S_1,A_1) + ... \mid S_0 = s,A_0 = a]) \\
&= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}(r(s_0,a_0) + \alpha{\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}({\mathbb{E}}_{\tau \sim \pi} [r(S_1,A_1) + \alpha r(S_2,A_2) ... \mid S_0 = s,A_0 = a,S_1 = s^\prime])) \\
&= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}(r(s_0,a_0) + \alpha{\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}({\mathbb{E}}_{\tau_{\geq 1} \sim \pi} [r(S_1,A_1) + \alpha r(S_2,A_2)+ ... \mid S_1 = s^\prime])) \\
&= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}[r(s_0,a_0) + \alpha{\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}(V_\pi(s^\prime))] \\
&= \sum_a \pi (a \mid s) [r(s,a)+\alpha \sum_{s^\prime} p(s^\prime \mid s,a)V_\pi(s^\prime)]
\end{align*}
$$

同理

$$
\begin{align*}
Q_\pi(s,a) &= {\mathbb{E}}_{\tau \sim \pi}[r(S_0,A_0) + \alpha r(S_1,A_1) + ... \mid S_0 = s,A_0 = a] \\
&= r(s,a) + \alpha {\mathbb{E}}_{\tau \sim \pi}[r(S_1,A_1) + \alpha r(S_2,A_2) + ... \mid S_0 = s,A_0 = a] \\
&= r(s,a) + \alpha {\mathbb{E}}_{s^\prime,a^\prime}[{\mathbb{E}}_{\tau_{\geq 1} \sim \pi}[r(S_1,A_1) + \alpha r(S_2,A_2) + ... \mid S_1 = s^\prime,A_1 = a^\prime]] \\
&= r(s,a) + \alpha {\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a),a^\prime \sim \pi(\cdot \mid s^\prime)}[Q_\pi (s^\prime,a^\prime)] \\
&= r(s,a) + \alpha {\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}[V_\pi(s^\prime)]
\end{align*}
$$

最优值函数

$$
\begin{align*}
V^\ast(s) &= \max_a Q^\ast(s,a)\\
&= \max_a (r(s,a) + {\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}[V^\ast(s^\prime)])
\end{align*}
$$

$$
Q^\ast(s,a) = {\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}(r(s,a) + \alpha \max_{a^\prime} Q^\ast(s^\prime,a^\prime))
$$

## 蒙特卡洛方法

设 $X \sim f(x)$, $X_1, X_2 ,..., X_n$ 是在这个概率分布下的独立同分布采样。

随机变量的期望与方差为

$$
{\mathbb{E}}(X) = \mu_X \\
Cov(X) = Q_X
$$

定义采样均值与采样方差

$$
\begin{align*}
\bar {X}_n &= \frac{1}{n}\sum_i X_i = \frac{X_1 + X_2 + ... X_n}{n} \\
\bar Q_n &= \frac{1}{n-1} \sum_i (X_i- \bar X_n)^T(X_i- \bar X_n)
\end{align*}
$$

采样均值与方差最终会收敛到

$$
{\mathbb{E}}(\bar X_n) = \mu_X \\
{\mathbb{E}}(\bar Q_n) = Q_X
$$

中心极限定理，$\bar X_n$也是一个新的随机变量，随着$n$的增长

$$
\lim_{n \rightarrow +\infty} \bar X_n \sim \mathcal{N}(\mu_X,\frac{Q_X}{n})
$$

### 使用蒙特卡洛估计期望

期望 ${\mathbb{E}}(\phi(X))$ 可以使用如下式子进行估计

$$
\frac{1}{n}\sum_i \phi(X_i) \approx E(\phi(X))
$$

其中 $X_i \sim f_X(x)$ 是独立同分布采样.

### 重要性采样

假设需要估计 ${\mathbb{E}}_g(X) = \sum_x xg(x)$ 也就是说，$X$ 是在 $g(x)$ 下的采样的，但我们只有 $f(x)$ 的采样知识,

$$
\begin{align*}
{\mathbb{E}}_g(\phi(X)) &= \sum_x \phi(x)g(x) \\
&= \sum_x \phi(x) \frac{g(x)}{f(x)} f(x) \\
&= {\mathbb{E}}_f(\phi(x)\frac{g(x)}{f(x)}) \\

{\mathbb{E}}_f(\phi(x)\frac{g(x)}{f(x)}) &\approx \frac{1}{N} \sum_i \frac{g(X_i)}{f(X_i)}\phi(X_i) \quad X_i \sim f(x)
\end{align*}
$$

## 策略梯度Policy Gradient

使用参数向量 $\theta$ 参数化策略 $\pi$, 称为 $\pi_\theta := \pi_\theta(\cdot \mid s)$.目前通常使用一个神经网络拟合它.

在 $\pi_\theta$ 的策略下轨迹 $\tau$ 的可能性称为 $P_\theta(\tau)$，根据概率乘法性质与马尔可夫性

$$
\begin{align*}
P_\theta(\tau) &= p(s_0)\pi_\theta(a_0 \mid s_0)p(s_1 \mid s_0,a_0)\pi_\theta(a_1 \mid s_1)... \\
&= p(s_0) \prod_{t=0}^{T-1} \pi_\theta(a_t \mid s_t)p(s_{t+1} \mid a_t,s_t)
\end{align*}
$$

求最优的策略相当于一个优化问题

$$
\begin{align*}
\max_\theta U(\theta) &= \max_\theta{\mathbb{E}}_{\tau\sim P_\theta(\tau)}[R(\tau)] \\
&= \max_\theta \sum_\tau P_\theta(\tau)R(\tau)
\end{align*}
$$

那么便可以使用一阶梯度，进行梯度上升求解.

$$
\theta^+ = \theta + \beta \nabla_\theta U(\theta)
$$

### 策略梯度估计

使用蒙特卡洛方法

$$
U(\theta) = \sum_\tau P_\theta(\tau)R(\tau) \approx \frac{1}{N} \sum_i R(\tau_i)
$$

$$
\begin{align*}
  \nabla_\theta U(\theta) &= \frac{\partial}{\partial\theta} \sum_\tau P_\theta(\tau)R(\tau) \\
  &= \sum_\tau \frac{\partial P_\theta(\tau)}{\partial \theta} R(\tau) \\
\end{align*}
$$

但是$\frac{\partial P_\theta(\tau)}{\partial \theta}$ 不是概率分布函数，使用重要性采样原理

$$
\begin{align*}
\nabla_\theta U(\theta) &= \sum_\tau \frac{\partial P_\theta(\tau)}{\partial \theta} R(\tau) \\
&= \sum_\tau P_\theta(\tau) \frac{\partial P_\theta(\tau)}{P_\theta(\tau)\partial \theta} R(\tau) \\
&= \sum_\tau P_\theta(\tau) \nabla_\theta \log P_\theta(\tau)R(\tau) \\
&= {\mathbb{E}}_{\tau \sim P_\theta(\tau)}(\nabla_\theta \log P_\theta(\tau)R(\tau))
\end{align*}
$$

使用蒙特卡洛方法

$$
\nabla_\theta U(\theta) \approx \frac{1}{N} \sum_i \nabla_\theta \log P_\theta(\tau^{(i)})R(\tau^{(i)})
$$

此外

$$
\begin{align*}
\nabla_\theta \log P_\theta(\tau) &= \nabla_\theta(\log p(s_0) + \sum_{t=0}^{T-1}[\log \pi_\theta(a_t \mid s_t)+\log p(s_{t+1} \mid a_t, a_t)]) \\
&= \nabla_\theta \sum_{t=0}^{T-1}\log \pi_\theta(a_t \mid s_t)
\end{align*}
$$

所以

$$
\nabla_\theta U(\theta) \approx \frac{1}{N} \sum_i (\nabla_\theta \sum_{t=0}^{T-1}\log \pi_\theta(a^{(i)}_t \mid s^{(i)}_t) R(\tau^{(i)}))
$$

## 价值函数估计

估计在当前策略下的价值函数与动作函数的值，根据贝尔曼方程

$$
\begin{align*}
V_\pi(s) &= \sum_a \pi (a \mid s) [r(s,a)+\alpha \sum_{s^\prime} p(s^\prime \mid s,a)V_\pi(s^\prime)] \\
Q_\pi(s) &= r(s,a) + \alpha \sum_{s^\prime} p(s^\prime \mid s,a) \sum_{a^\prime}\pi(a^\prime \mid s^\prime) Q_\pi(s^\prime,a^\prime)
\end{align*}
$$

### 蒙特卡洛方法

进行多次试验，使用价值函数的定义式来估计值函数

$$
\begin{align*}
  V_\pi(s) &\triangleq {\mathbb{E}}_{\tau \sim \pi}[R(\tau) \mid S_0 = s] \\
  Q_\pi(s,a) &= {\mathbb{E}}_{\tau \sim \pi}[r(S_0,A_0) + \alpha r(S_1,A_1) + ... \mid S_0 = s,A_0 = a]
\end{align*}
$$

价值函数估计大致流程如下

1. 采样完整轨迹(Episode),从初始状态出发，遵循策略 $\pi$与环境进行交互，获得

    $$
    (s_0,a_0,r_0),(s_1,a_1,r_1),(s_2,a_2,r_2),...,(s_{T-1},a_{T-1},r_{T-1}),..,(s_T)
    $$

2. 从后往前运行，计算当前状态$s_{t}$折扣回报并记录

    $$
    G(s_{t}) = r_{t} + \gamma G(s_{t+1})
    $$

3. 遍历所有的状态空间 $S$，计算对应状态的折扣回报

    $$
    V_\pi(s) = \frac{1}{k}\sum_{i=1}^k G^{(m)}(s)
    $$

动作价值函数估计大致如此.

### 增量式更新

使用增量式更新可以避免存储所有的折扣回报

$$
V_\pi^{(m+1)}(s_t) = V_\pi^{(m)}(s_t) + \frac{1}{m+1}(G_t^{m+1}- V_\pi^{(m)}(s_t))
$$

其中，$V_\pi^{(m+1)}(s_t)$ 表示第 $m+1$ 次更新状态 $s_t$的折扣回报，$G_t^{m+1}(s_t)$ 表示第 $m+1$ 次更新的状态 $s_t$ 的折扣函数.

$$
\begin{align*}
  V_\pi^{(m+1)}(s_t) &= \frac{1}{m+1} \sum_i^{m+1} G_t^{(i)} \\
  &= \frac{1}{m+1}(\sum_i^m G^{(i)}_t + G^{(m+1)}_t) \\
  &= \frac{m}{m+1}V_\pi^{(m)}(s_t) + \frac{1}{m+1}G^{(m+1)}_t \\
  &= V_\pi^{(m)}(s_t) + \frac{1}{m+1}(G_t^{m+1}- V_\pi^{(m)}(s_t))
\end{align*}
$$

更通用的可以总结为

$$
新估计 = 旧估计 + \alpha (新观测 - 旧估计)
$$

### 时序差分TD

蒙特卡洛方法的缺陷在： 它需要完整的轨迹来重构当前观测的 $G_t$，但如果是十分长的轨迹序列，蒙特卡洛方法可能便不再适用.

称 $r_t + \gamma \hat V_\pi(s_{t+1})$ 为 `TD target`.

称 $r_t + \gamma \hat V_\pi(s_{t+1})- V_\pi^{(m)}(s_t)$ 为 `TD error`

时序差分的方法，用来估计当前观测 $G_t$

$$
\begin{align*}
  G_t &= r_t + \gamma r_{t+1} + \gamma^2 r_{t+2} + ... \\
  &= r_t + \gamma(r_{t+1} + \gamma r_{t+2}) \\
  &= r_t + \gamma G_{t+1} \\
  & \approx r_t + \gamma \hat V_\pi(s_{t+1})
\end{align*}
$$

也可以理解为贝尔曼方程

$$
\begin{align*}
V_\pi(s_t) &= {\mathbb{E}}_{a \sim \pi(\cdot \mid s)}[r_t + \gamma{\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a)}(V_\pi(s_{t+1}))] \\
& \approx r_t + \gamma \hat V_\pi(s_{t+1})
\end{align*}
$$

一步返回的TD公式为

$$
V_\pi^{(m+1)}(s_t) = V_\pi^{(m)}(s_t) +  \alpha ( r_t + \gamma \hat V_\pi(s_{t+1})- V_\pi^{(m)}(s_t))
$$

### n步时序差分

使用$n$步来估计 $G_t$

$$
\begin{align*}
  G_t &\approx r_t + \gamma \hat V_\pi(s_{t+1}) \quad &\text{1-step}\\
  G_t  &\approx r_t + \gamma r_{t+1} + \gamma^2 \hat V_\pi(s_{t+2}) \quad &\text{2-step} \\
  G_t &\approx r_t + \gamma r_{t+1} + ...+ \gamma^{n-1}r_n + \gamma^n \hat V_\pi(s_{t+n}) \quad &\text{n-step}
\end{align*}
$$

将这个公式带回到增量式更新即可。

蒙特卡洛算法可以理解为无穷步的时序差分.

## 动作价值函数估计

蒙特卡洛方法

$$
\begin{align*}
  Q_\pi^{(m+1)}(s_t,a_t) = \frac{1}{m+1}\sum_{i=1}^{m+1}G_t^{(i)}
\end{align*}
$$

蒙特卡洛增量方法

$$
Q_\pi^{(m+1)}(s_t,a_t) = Q_\pi^{(m)}(s_t,a_t) + \frac{1}{m+1}(G_t^{(m+1)}-Q_\pi^{(m)}(s_t,a_t))
$$

1步时序差分

$$
G_t^{(m+1)} \approx r_t + \gamma \hat Q_\pi(s_{t+1},a_{t+1}) \\
Q_\pi^{(m+1)}(s_t,a_t) = Q_\pi^{(m)}(s_t,a_t) + \alpha(r_t + \gamma \hat Q_\pi(s_{t+1},a_{t+1})-Q_\pi^{(m)}(s_t,a_t))
$$

n步时序差分

$$
G_t^{(m+1)} \approx r_t + \gamma r_{t+1} + ...+ \gamma^{n-1}r_n + \gamma^n \hat Q_\pi(s_{t+n},a_{t+n})
$$

## $\lambda$回报

`λ-return`（λ-回报）是强化学习中时序差分（TD）学习的核心概念，用于在偏差（Bias）与方差（Variance）之间实现平衡，是TD(λ)算法的基础。数学表达式为

$$
G_t^\lambda = (1-\lambda) \sum_{n=1}^\infty \lambda^{n-1}G_t^{(n)}
$$

$G_t^{(n)}$ n步回报. $\lambda \in (0,1)$是加权系数

将每一步带入进去，获得

$$
G_t^\lambda =\sum_{l=1}^\infty(\gamma\lambda)^{l-1}r_{t+l} + \gamma^l \lambda^{l-1}((1-\lambda)\hat V_\pi(s_{t+l}))
$$

可见，这是一个加权，$\lambda$ 越大偏差越大，但方差越小。

## 参数化

如果状态数很大，甚至是无穷多的状态，那么就应该使用一个神经网络进行拟合.将状态数用神经网络参数拟合.

使用神经网络参数化价值函数，

$$
\hat V_\pi(s,w)
$$

$w$是神经网络的参数.

优化的目标便是寻找 $w^\ast$ 使得

$$
w^\ast = \argmin E_\pi [(y_t - \hat V_\pi(s_t,w))^2]
$$

其中 $y_t$ 是真实的价值函数值，但是难以得到，所以采用 `TD Target`近似表示

使用均方误差作为损失函数

$$
\mathbb{L}(w) = \frac{1}{n}\sum_i(r_{t+1}^{(i)} + \gamma V_\pi(s_{t+1}^{(i)},w)-\hat V\pi(s_t,w)^2)
$$

## 策略提升

回顾策略梯度，并进行数学变换

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[G_0(\tau) \sum_{t=0}^{T-1}\nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

无穷步的轨迹为

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[G_0(\tau) \sum_{t=0}^{\infty}\nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

其中

$$\begin{align*}
  J(\pi_\theta) &= {\mathbb{E}}_{\tau\sim P_\theta(\tau)}[R(\tau)] \\
  G_0(\tau) &= \sum_{k=0}^{\infty} \gamma^k r_k
\end{align*}
$$

可以证明

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\sum_{t=0}^{\infty}\nabla_\theta\log \pi_\theta(a_t \mid s_t) G_t(\tau)]
$$

其中

$$
G_t(\tau) = \sum_{k=t}^{\infty} \gamma^{k-t}r_k
$$

可以理解为把 $t$ 时刻认为零时刻，开始计算累计函数.

证明过程，就是证明

$$
{\mathbb{E}}_{\tau \sim P_\theta(\tau)}[G_{< t}(\tau) \nabla_\theta\log \pi_\theta(a_t \mid s_t)] = 0
$$

展开对数概率

$$
\begin{align*}
  原式 &= {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[G_{< t}(\tau) \frac{\nabla_\theta \pi_\theta(a_t \mid s_t)}{\pi_\theta(a_t \mid s_t)}] \\
  &= \sum_\tau G_{< t}(\tau) \frac{\nabla_\theta \pi_\theta(a_t \mid s_t)}{\pi_\theta(a_t \mid s_t)} p(\tau) \\
  &= 0
\end{align*}
$$

还可以将概率梯度使用动作价值函数表示

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\sum_{t=0}^{\infty}Q_\pi(s_t,a_t) \nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

可以使用全期望公式证明

$$
\begin{align*}
{\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\nabla_\theta\log \pi_\theta(a_t \mid s_t) G_t(\tau)] &=  {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\nabla_\theta\log \pi_\theta(a_t \mid s_t) G_t(\tau)] \\
&= {\mathbb{E}}_{s_t,a_t}{\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\nabla_\theta\log \pi_\theta(a_t \mid s_t) G_t(\tau) \mid s_t,a_t] \\
&= {\mathbb{E}}_{s_t,a_t}\nabla_\theta\log \pi_\theta(a_t \mid s_t) {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[G_t(\tau) \mid s_t,a_t] \\
&= {\mathbb{E}}_{s_t,a_t} \nabla_\theta\log \pi_\theta(a_t \mid s_t) Q_\pi(s_t,a_t) \\
\end{align*}
$$

### 总结

通用公式

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\sum_{t=0}^{\infty}f_t \nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

其中 $f_t$可以是

$$
\begin{align*}
  & G_t(\tau) \\
  & G_t(\tau) - V_\pi(s_t) \\
  & Q(s_t,a_t) \\
  & Q(s_t,a_t) - V_\pi(s_t) \\
  & r_t + \gamma V_\pi(s_{t+1}) - V_\pi(s_t)
\end{align*}
$$

还可以是`n`步TD都可以.

## Actor-Critic

使用两个神经网络，一个估计策略 $\pi_\theta(a \mid s)$,一个估计价值函数 $V_\phi(s)$.两个神经网络的协同工作.在线(on policy)优化策略与估计价值函数

### Actor（策略网络）

* **功能**：策略网络根据当前状态 $s$,输出动作的概率分布 $\pi_\theta(a\mid s)$
* **作用**: 根据当前策略选择动作，探索环境并收集经验
* **更新方式**： 通过策略梯度（Policy Gradient）调整参数 $\theta$，目标是最大化累积奖励的期望

策略网络的目标是最大化策略价值

$$
\max_\theta J(\pi_\theta)
$$

变为最小化以适应标准优化问题

$$
\min_\theta -J(\pi_\theta)
$$

已知策略梯度

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim P_\theta(\tau)}[\sum_{t=0}^{\infty}f_t \nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

损失函数就是

$$
L_{actor} = -{\mathbb{E}}[\sum_{t=0}^{\infty}f_t \log \pi_\theta (a_t \mid s_t)]
$$

### Critic（价值网络）

* **功能**： 输入当前状态 $s$,输出状态价值 $V_\phi(s)$ 标量，表示该状态的好坏。
* **作用**：评估Actor所选动作的价值，为Actor提供即时反馈（替代REINFORCE的“整局回报”），降低策略更新的方差。
* **更新方式**： 通过时序差分（TD, Temporal-Difference）学习调整参数 $\phi$，目标是最小化预测值与真实值的误差.

价值网络的目标是让TD误差为零

$$
\delta = (r_t + \gamma V_\phi(s_{t+1}) - V_\phi(s_t))
$$

使用`MSE`损失函数，最小化损失函数

$$
L_{critic} = {\mathbb{E}}[\delta^2] =  {\mathbb{E}}[(r_t + \gamma V_\phi(s_{t+1}) - V_\phi(s_t))^2]
$$

#### 注意点

* $r_t + \gamma V_\phi(s_{t+1})$被认为是真值，尽管他是估计出来的，但在这一步的优化中，不应该受到影响，对应到强化学习就是使用`detach`截断梯度.

### 常用算法

#### 标准的TD Actor-Critic

$$
\begin{align*}
  &初始化策略网络 \quad \pi_\theta(a\mid s) \\
 & 初始化价值网络 \quad V_\phi(s) \\
& 对每个轨迹中的 \quad t \quad 执行 \\
&\quad 按照策略 \pi_\theta(a_t \mid s_t) 生成动作\; a_t \\
&\quad \delta = r_t + \gamma V_\phi(s_{t+1}) - V_\phi(s_t) \\
&\quad \phi = \phi + \alpha_\phi \delta \nabla V_\phi(s_t) \\
&\quad \theta = \theta + \alpha_\theta \nabla \ln \pi_\theta (a_t \mid s_t)
\end{align*}
$$

但是，这样轨迹利用率太低了，使用重要性采样转化

$$
\nabla_\theta J(\pi_\theta) = {\mathbb{E}}_{\tau \sim \pi_{\theta old}}[\sum_{t=0}^{\infty}\frac{\pi_\theta(a_t \mid s_t)}{\pi_{\theta old}(a_t \mid s_t)}f_t \nabla_\theta\log \pi_\theta(a_t \mid s_t)]
$$

#### Proximal Policy Optimization PPO

通过重要性采样+限制策略更新幅度，便是PPO算法.

此时的损失函数为,最大化这个函数

$$
{\mathbb{E}}_{\tau \sim \pi_k}[\frac{\pi_\theta(a_t \mid s_t)}{\pi_{\theta_k}(a_t \mid s_t)}A_{\pi_{\theta_k}}(s_t,a_t)]
$$

PPO算法便是添加了一个裁剪项，防止前后策略变化过大.

$$
L(s,a,\theta_k,\theta) = \min (\frac{\pi_\theta(a_t \mid s_t)}{\pi_{\theta_k}(a_t \mid s_t)}A_{\pi_{\theta_k}}(s_t,a_t),clip(\frac{\pi_\theta(a_t \mid s_t)}{\pi_{\theta_k}(a_t \mid s_t)},1-\epsilon,1+\epsilon)A_{\pi_{\theta_k}}(s_t,a_t))
$$

或者是

$$
L(s,a,\theta_k,\theta) = \min (\frac{\pi_\theta(a_t \mid s_t)}{\pi_{\theta_k}(a_t \mid s_t)}A_{\pi_{\theta_k}}(s_t,a_t),g(\epsilon,A_{\pi_{\theta_k}}(s_t,a_t))) \\
g(\epsilon,A(s_t,a_t)) =
\begin{cases}
  (1+\epsilon), \quad &A_{\pi_{\theta_k}}(s_t,a_t) > 0 \\
  (1-\epsilon), \quad & A_{\pi_{\theta_k}}(s_t,a_t)(s_t,a_t) < 0
\end{cases}
$$

优势函数 $A(s_t,a_t)$ 表示当前动作对在旧策略下的好坏程度，这个表示，新策略只能比就旧策略好 $1+\epsilon$, 或者坏 $1-\epsilon$.

优势函数可以是n步的TD error

$$
A_t^{(n)} = r_t + \gamma r_{t+1} + \gamma^2 r_{t+2} + ... \gamma^nr_{t+n} - V(s_t)
$$

可以用一步的td error表示n步的td error，这个公式更加像动作价值函数的td估计

$$
\begin{align*}
  \delta_t &= r_t + \gamma V(s_{t+1}) - V(s_t) \\
  A_t^{(1)} &=  \delta_t \\
  A_t^{(2)} &= \delta_t + \gamma \delta_{t+1} \\
  A_t^{(3)} &= \delta_t + \gamma \delta_{t+1} + \gamma^2 \delta_{t+2}
\end{align*}
$$

也可以是 TD($\lambda$).

$$
\hat A_t = (1-\lambda)\sum_k \lambda^{k-1} A_t^{(k)}
$$

它的递推公式是

$$
\hat A_t = \delta_t + \gamma \lambda \hat A_{t+1}
$$

#### 熵正则化

强化学习中一种用来鼓励探索 (Exploration)、防止模型过早陷入局部最优的技术。

熵公式为

$$
H(\pi) = - \sum \pi(a|s) \log \pi(a|s)
$$

修改期望函数$J(\pi_\theta)$添加熵

$$
J(\theta) = \mathbb{E} [ \text{Reward} + \beta \cdot \text{Entropy} ]
$$

* $\beta$是系数，通常为`0.01`,越大，表示期望输出策略的熵越大.
