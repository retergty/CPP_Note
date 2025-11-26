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
G_t = r_t + \gamma r_{t+1} + \gamma^2 r_{t+1} + ... = \sum_{k=0}^\infty \gamma^k r_{t+k}
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
&= r(s,a) + \alpha {\mathbb{E}}_{s^\prime \sim p(\cdot \mid s,a),a^\prime \sim \pi(\cdot \mid s)}[Q_\pi (s^\prime,a^\prime)] \\
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

### 估计策略梯度

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

