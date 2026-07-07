# llm 基础知识

## RMSNorm

$$
rms(x) = \sqrt{\frac{1}{d} \sum_{i=1}^{d} x_i^2 + \epsilon}
$$

`RMSNorm`，即 `Root Mean Square Layer Normalization`，使用向量的均方根进行归一化。公式如下：

$$
RMSNorm(x) = \frac{x}{rms(x)} \odot g
$$

其中：

- $x \in \mathbb{R}^{d}$ 表示一个 token 的隐藏状态向量。
- $d$ 是隐藏层维度。
- $\epsilon$ 是一个很小的常数，用于避免除零。
- $g \in \mathbb{R}^{d}$ 是可学习的缩放参数，通常也叫 `weight`。
- $\odot$ 表示逐元素相乘。

通常在 LLM 中，输入张量维度是 $(B, T, C)$：

- $B$ 表示 batch size。
- $T$ 表示 sequence length。
- $C$ 表示 hidden size。

`RMSNorm` 会沿着最后一维 $C$ 计算 `RMS`。也就是说，每个 batch 中的每个 token 都会独立计算自己的均方根：

$$
rms(x_{b,t}) = \sqrt{\frac{1}{C} \sum_{i=1}^{C} x_{b,t,i}^{2} + \epsilon}
$$

然后对该 token 的隐藏向量做归一化：

$$
y_{b,t,i} = \frac{x_{b,t,i}}{rms(x_{b,t})} \cdot g_i
$$

## LayerNorm

`LayerNorm`，即 `Layer Normalization`，会对一个向量先减去均值，再除以标准差，使归一化后的向量均值接近 0，方差接近 1。

对于一个 token 的隐藏状态向量 $x \in \mathbb{R}^{d}$：

$$
mean(x) = \frac{1}{d} \sum_{i=1}^{d} x_i
$$

$$
var(x) = \frac{1}{d} \sum_{i=1}^{d} (x_i - mean(x))^2
$$

`LayerNorm` 的公式如下：

$$
LayerNorm(x) = \frac{x - mean(x)}{\sqrt{var(x) + \epsilon}} \odot g + b
$$

其中：

- $g \in \mathbb{R}^{d}$ 是可学习的缩放参数，通常也叫 `weight` 或 $\gamma$。
- $b \in \mathbb{R}^{d}$ 是可学习的偏置参数，通常也叫 `bias` 或 $\beta$。
- $\epsilon$ 是一个很小的常数，用于避免除零。

如果输入张量维度是 $(B, T, C)$，`LayerNorm` 通常也是沿着最后一维 $C$ 做归一化。也就是说，每个 batch 中的每个 token 都会独立计算自己的均值和方差：

$$
mean(x_{b,t}) = \frac{1}{C} \sum_{i=1}^{C} x_{b,t,i}
$$

$$
var(x_{b,t}) = \frac{1}{C} \sum_{i=1}^{C} (x_{b,t,i} - mean(x_{b,t}))^2
$$

然后对该 token 的隐藏向量做归一化：

$$
y_{b,t,i} = \frac{x_{b,t,i} - mean(x_{b,t})}{\sqrt{var(x_{b,t}) + \epsilon}} \cdot g_i + b_i
$$

因此，`LayerNorm` 不依赖 batch 内其他样本，也不依赖其他 token。它只使用当前 token 自己的 hidden vector 来计算归一化统计量。

### LayerNorm 在 Transformer 中的作用

`LayerNorm` 常用于 Transformer block 的注意力层和 FFN 层附近，用来稳定训练过程，让不同层之间的激活值分布更平稳。

常见结构有两种：

- `Post-LN`：先执行子层，再做残差连接和 `LayerNorm`。
- `Pre-LN`：先做 `LayerNorm`，再执行子层和残差连接。

现代 LLM 更多使用 `Pre-LN` 风格，因为深层网络训练通常更稳定。很多 LLM 会把这里的 `LayerNorm` 替换成更轻量的 `RMSNorm`。

## RMSNorm 和 LayerNorm 的区别

而 `RMSNorm` 不减均值，只用均方根进行缩放：

$$
RMSNorm(x) = \frac{x}{\sqrt{mean(x^2) + \epsilon}} \odot g
$$

所以 `RMSNorm` 计算更简单，速度更快，并且在很多 LLM 中效果足够好，例如 `LLaMA` 系列模型就使用了 `RMSNorm`。

## 自注意力机制 self-attention

给定输入矩阵 $X \in \mathbb{R}^{T \times C}$，其中：

- $T$ 表示 sequence length。
- $C$ 表示 hidden size。

自注意力的核心思想是：序列中的每个 token 根据自身与序列内其他 token 的相关性，对序列信息进行加权聚合。

### 1. 生成 Q、K、V

首先通过三个线性变换，把输入 $X$ 映射成 `Query`、`Key`、`Value`：

$$
Q = XW_Q
$$

$$
K = XW_K
$$

$$
V = XW_V
$$

其中：

- $Q$ 表示 query，用于计算当前 token 与其他 token 的相关性。
- $K$ 表示 key，用于提供被匹配的特征。
- $V$ 表示 value，用于提供被聚合的信息。

线性映射矩阵的维度为：

$$
W_Q, W_K \in \mathbb{R}^{C \times d_k}
$$

$$
W_V \in \mathbb{R}^{C \times d_v}
$$

因此：

$$
Q, K \in \mathbb{R}^{T \times d_k}
$$

$$
V \in \mathbb{R}^{T \times d_v}
$$

在理解上，`Q`、`K`、`V` 可以看作是对输入序列 $X$ 的线性变换。对于序列中的每个 `token`，其原始 `hidden vector` 维度为 $C$，经过不同的线性映射后，分别投影到 $d_k$ 维的`query/key`空间和 $d_v$ 维的`value`空间中。

### 2. 计算注意力分数

对每个 token 的 query 和所有 token 的 key 做点积，得到注意力分数：

$$
Q \in \mathbb{R}^{T \times d_k}
$$

$$
K \in \mathbb{R}^{T \times d_k}
$$

注意力分数矩阵为：

$$
\begin{align*}
S &= QK^T \\
&= \begin{bmatrix}
Q_1 \\ Q_2 \\ \vdots \\ Q_T
\end{bmatrix}\begin{bmatrix}
    K_1^T & K_2^T & \cdots & K_T^T
\end{bmatrix} \\
&= \begin{bmatrix}
Q_1K_1^T & Q_1K_2^T & \cdots & Q_1K_T^T \\
Q_2K_1^T & Q_2K_2^T & \cdots & Q_2K_T^T \\
\vdots & \vdots & \ddots & \vdots \\
Q_TK_1^T & Q_TK_2^T & \cdots & Q_TK_T^T
\end{bmatrix}
\end{align*}
$$

其中 $Q_i, K_i \in \mathbb{R}^{1 \times d_k}$，分别表示第 $i$ 个 token 的 query 向量和 key 向量。

因此：

$$
S \in \mathbb{R}^{T \times T}
$$

元素 $S_{i,j}$ 为：

$$
S_{i,j} = Q_iK_j^T = \sum_{r=1}^{d_k} Q_{i,r}K_{j,r}
$$

缩放点积注意力使用 $\sqrt{d_k}$ 对注意力分数进行缩放：

$$
\hat{S}_{i,j} = \frac{S_{i,j}}{\sqrt{d_k}}
$$

其中 $\hat{S}_{i,j}$ 表示缩放后的注意力分数。

此时缩放后的注意力分数矩阵为：

$$
\hat{S} \in \mathbb{R}^{T \times T}
$$

其中 $\hat{S}_{i,j}$ 表示第 $i$ 个 token 对第 $j$ 个 token 的注意力分数。

### softmax 函数

`softmax` 函数将一个实数向量转换为概率分布。

给定向量 $z \in \mathbb{R}^{n}$：

$$
z = [z_1, z_2, \cdots, z_n]
$$

`softmax` 的第 $i$ 个输出为：

$$
softmax(z)_i = \frac{e^{z_i}}{\sum_{j=1}^{n} e^{z_j}}
$$

输出向量满足：

$$
softmax(z)_i > 0
$$

$$
\sum_{i=1}^{n} softmax(z)_i = 1
$$

### 3. 使用 softmax 得到注意力权重

对注意力分数的最后一维做 `softmax`，把分数转换成概率分布：

$$
A = softmax(\hat{S})
$$

其中：

$$
A \in \mathbb{R}^{T \times T}
$$

对于每个 token 来说，它对所有 token 的注意力权重之和为 1。

### 4. 根据注意力权重聚合 V

最后用注意力权重对 $V$ 做加权求和：

$$
A \in \mathbb{R}^{T \times T}
$$

$$
V \in \mathbb{R}^{T \times d_v}
$$

$$
O = AV
$$

输出矩阵形状为：

$$
O \in \mathbb{R}^{T \times d_v}
$$

也就是说，每个 token 的输出向量都是所有 token 的 value 向量的加权和。

完整公式可以写成：

$$
Attention(Q, K, V) = softmax(\frac{QK^T}{\sqrt{d_k}})V
$$

## 多头注意力机制 Multi-Head Attention

单独只有一个注意力矩阵可能无法捕捉到序列中不同子空间的特征，因此引入了多头注意力机制。

给定输入 $X \in \mathbb{R}^{B \times T \times C}$，其中：

- $B$ 表示批处理数`batch size`。
- $T$ 表示序列长度`sequence length`。
- $C$ 表示隐藏维度`hidden size`。
