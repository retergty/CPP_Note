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

## 因果注意力 Causal Attention

`Causal Attention` 在自注意力的注意力分数上加入因果 mask，使第 $i$ 个 token 只能关注第 $1$ 到第 $i$ 个 token，不能关注第 $i+1$ 到第 $T$ 个 token。

给定：

$$
Q, K \in \mathbb{R}^{T \times d_k}
$$

$$
V \in \mathbb{R}^{T \times d_v}
$$

先计算缩放注意力分数：

$$
S = \frac{QK^T}{\sqrt{d_k}}
$$

其中：

$$
S \in \mathbb{R}^{T \times T}
$$

元素 $S_{i,j}$ 表示第 $i$ 个 token 对第 $j$ 个 token 的注意力分数。

### 1. 构造因果 mask

定义因果 mask 矩阵：

$$
M \in \mathbb{R}^{T \times T}
$$

其中：

$$
M_{i,j} =
\begin{cases}
0, & j \le i \\
-\infty, & j > i
\end{cases}
$$

矩阵形式为：

$$
M =
\begin{bmatrix}
0 & -\infty & -\infty & \cdots & -\infty \\
0 & 0 & -\infty & \cdots & -\infty \\
0 & 0 & 0 & \cdots & -\infty \\
\vdots & \vdots & \vdots & \ddots & \vdots \\
0 & 0 & 0 & \cdots & 0
\end{bmatrix}
$$

其中 $j > i$ 的位置表示未来 token，对应的注意力分数被屏蔽。

### 2. 加入 mask

将 mask 加到注意力分数上：

$$
S^{mask} = S + M
$$

因此：

$$
S^{mask}_{i,j} =
\begin{cases}
S_{i,j}, & j \le i \\
-\infty, & j > i
\end{cases}
$$

### 3. masked softmax

对 $S^{mask}$ 的最后一维做 `softmax`：

$$
A = softmax(S^{mask})
$$

对于 $j > i$ 的位置：

$$
e^{-\infty} = 0
$$

因此：

$$
A_{i,j} = 0,\quad j > i
$$

每个 token 的注意力权重只分布在自身和历史 token 上：

$$
\sum_{j=1}^{i} A_{i,j} = 1
$$

### 4. 聚合 V

使用因果注意力权重对 $V$ 做加权求和：

$$
O = AV
$$

其中：

$$
O \in \mathbb{R}^{T \times d_v}
$$

第 $i$ 个 token 的输出为：

$$
O_i = \sum_{j=1}^{i} A_{i,j}V_j
$$

完整公式为：

$$
CausalAttention(Q, K, V) = softmax(\frac{QK^T}{\sqrt{d_k}} + M)V
$$

### 5. 在多头注意力中的 mask 形状

在 MHA 或 GQA 中，注意力分数通常具有如下形状：

$$
S \in \mathbb{R}^{B \times H \times T \times T}
$$

因果 mask 的基础形状为：

$$
M \in \mathbb{R}^{T \times T}
$$

计算时，$M$ 会广播到 batch 维和 head 维：

$$
M \in \mathbb{R}^{1 \times 1 \times T \times T}
$$

加到每个 batch、每个 head 的注意力分数上。

## GQA 多头注意力机制 Grouped Query Attention

单独只有一个注意力矩阵无法捕捉序列中不同子空间的特征，因此引入多头注意力机制。

给定输入 $X \in \mathbb{R}^{B \times T \times C}$，其中：

- $B$ 表示批处理数`batch size`。
- $T$ 表示序列长度`sequence length`。
- $C$ 表示隐藏维度`hidden size`。

设 `Query` 的头数为 $H$，`Key/Value` 的头数为 $H_{kv}$，且 $H$ 能被 $H_{kv}$ 整除。每个 `Query` head 的维度为：

$$
D = \frac{C}{H}
$$

GQA 中多个 `Query` head 共享同一个 `Key/Value` head。每个 `Key/Value` head 对应的 `Query` head 数量为：

$$
G = \frac{H}{H_{kv}}
$$

其中 $G$ 表示每组中的 `Query` head 数量。

### 1. 生成 Q、K、V

线性映射矩阵为：

$$
W_Q \in \mathbb{R}^{C \times (H \cdot D)}
$$

$$
W_K \in \mathbb{R}^{C \times (H_{kv} \cdot D)}
$$

$$
W_V \in \mathbb{R}^{C \times (H_{kv} \cdot D)}
$$

通过线性变换得到：

$$
Q = XW_Q
$$

$$
K = XW_K
$$

$$
V = XW_V
$$

然后将 $Q$、$K$、$V$ reshape 成多头形式：

$$
Q \in \mathbb{R}^{B \times T \times H \times D}
$$

$$
K \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$

$$
V \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$

### 2. Query head 到 Key/Value head 的分组映射

设 query head 下标为：

$$
h \in \{1, 2, \cdots, H\}
$$

第 $h$ 个 query head 对应的 key/value head 下标为：

$$
m(h) = \left\lceil \frac{h}{G} \right\rceil
$$

其中：

$$
m(h) \in \{1, 2, \cdots, H_{kv}\}
$$

也就是说，第 $1$ 到第 $G$ 个 query head 使用第 $1$ 个 key/value head，第 $G+1$ 到第 $2G$ 个 query head 使用第 $2$ 个 key/value head。

### 3. 每个 Query head 计算注意力

对第 $b$ 个 batch、第 $h$ 个 query head，有：

$$
Q_{b,h} \in \mathbb{R}^{T \times D}
$$

$$
K_{b,m(h)} \in \mathbb{R}^{T \times D}
$$

$$
V_{b,m(h)} \in \mathbb{R}^{T \times D}
$$

注意力分数为：

$$
S_{b,h} = \frac{Q_{b,h}K_{b,m(h)}^T}{\sqrt{D}}
$$

其中：

$$
S_{b,h} \in \mathbb{R}^{T \times T}
$$

注意力权重为：

$$
A_{b,h} = softmax(S_{b,h})
$$

其中 `softmax` 作用在 $S_{b,h}$ 的最后一维。

第 $h$ 个 query head 的输出为：

$$
O_{b,h} = A_{b,h}V_{b,m(h)}
$$

其中：

$$
O_{b,h} \in \mathbb{R}^{T \times D}
$$

### 4. 拼接所有 Query head 的输出

对第 $b$ 个 batch 内所有 query head 的输出进行拼接：

$$
O_b = concat(O_{b,1}, O_{b,2}, \cdots, O_{b,H})
$$

得到：

$$
O_b \in \mathbb{R}^{T \times (H \cdot D)}
$$

因为 $H \cdot D = C$，所以：

$$
O_b \in \mathbb{R}^{T \times C}
$$

对所有 batch 拼接后的输出张量为：

$$
O \in \mathbb{R}^{B \times T \times C}
$$

最后通过输出线性层：

$$
Y = OW_O
$$

其中：

$$
W_O \in \mathbb{R}^{C \times C}
$$

输出为：

$$
Y \in \mathbb{R}^{B \times T \times C}
$$

### GQA 和 MHA、MQA 的关系

- 当 $H_{kv} = H$ 时，每个 query head 都有独立的 key/value head，此时为普通 `Multi-Head Attention`。
- 当 $H_{kv} = 1$ 时，所有 query head 共享同一个 key/value head，此时为 `Multi-Query Attention`。
- 当 $1 < H_{kv} < H$ 时，多个 query head 按组共享 key/value head，此时为 `Grouped Query Attention`。

## 旋转位置编码 Rotary Position Embedding

基础的自注意力无法区分位置信息，通过旋转变换把位置信息注入到 `Query` 和 `Key` 中。

在自注意力中，位置编码需要影响注意力分数：

$$
S = QK^T
$$

因此 `RoPE` 作用在 $Q$ 和 $K$ 上，不作用在 $V$ 上。

### 1. 输入形式

对于单个 attention head，设：

$$
Q, K \in \mathbb{R}^{T \times D}
$$

其中：

- $T$ 表示序列长度。
- $D$ 表示每个 head 的维度。
- $D$ 为偶数。

第 $t$ 个 token 的 query 向量和 key 向量在本节中写成列向量，即 $Q$ 和 $K$ 第 $t$ 行的转置：

$$
q_t, k_t \in \mathbb{R}^{D \times 1}
$$

`RoPE` 会对 $q_t$ 和 $k_t$ 的相邻两个维度成对旋转。

### 2. 旋转角度

将向量维度按两维一组划分。第 $i$ 组维度为：

$$
(2i, 2i + 1)
$$

其中：

$$
i = 0, 1, \cdots, \frac{D}{2} - 1
$$

第 $i$ 组对应的频率为：

$$
\omega_i = \theta^{-\frac{2i}{D}}
$$

其中 $\theta$ 是 RoPE 的 base 参数。

第 $t$ 个位置在第 $i$ 组维度上的旋转角度为：

$$
\alpha_{t,i} = t \cdot \omega_i
$$

### 3. 二维旋转公式

对任意列向量 $x_t \in \mathbb{R}^{D \times 1}$，取第 $i$ 组二维子向量：

$$
\begin{bmatrix}
x_{t,2i} \\
x_{t,2i+1}
\end{bmatrix}
$$

RoPE 对该二维子向量做旋转：

$$
\begin{bmatrix}
\tilde{x}_{t,2i} \\
\tilde{x}_{t,2i+1}
\end{bmatrix}
=
\begin{bmatrix}
\cos \alpha_{t,i} & -\sin \alpha_{t,i} \\
\sin \alpha_{t,i} & \cos \alpha_{t,i}
\end{bmatrix}
\begin{bmatrix}
x_{t,2i} \\
x_{t,2i+1}
\end{bmatrix}
$$

展开为：

$$
\tilde{x}_{t,2i} = x_{t,2i}\cos \alpha_{t,i} - x_{t,2i+1}\sin \alpha_{t,i}
$$

$$
\tilde{x}_{t,2i+1} = x_{t,2i}\sin \alpha_{t,i} + x_{t,2i+1}\cos \alpha_{t,i}
$$

### 4. 对 Q 和 K 应用 RoPE

对每个位置 $t$：

$$
\tilde{q}_t = RoPE(q_t, t)
$$

$$
\tilde{k}_t = RoPE(k_t, t)
$$

得到：

$$
\tilde{Q}, \tilde{K} \in \mathbb{R}^{T \times D}
$$

注意力分数改为：

$$
S = \frac{\tilde{Q}\tilde{K}^T}{\sqrt{D}}
$$

元素级形式为：

$$
S_{i,j} = \frac{\tilde{q}_i^T \tilde{k}_j}{\sqrt{D}}
$$

其中 $\tilde{q}_i$ 带有第 $i$ 个位置的旋转信息，$\tilde{k}_j$ 带有第 $j$ 个位置的旋转信息。

### 5. RoPE 的相对位置性质

设 $R_t$ 表示第 $t$ 个位置对应的旋转矩阵。由于本节将 $q_i$ 和 $k_j$ 写成列向量，RoPE 通过左乘 $R_t$ 实现旋转：

$$
\tilde{q}_i = R_iq_i
$$

$$
\tilde{k}_j = R_jk_j
$$

注意力点积为：

$$
\tilde{q}_i^T\tilde{k}_j = (R_iq_i)^T(R_jk_j)
$$

因为旋转矩阵满足：

$$
R_i^T R_j = R_{j-i}
$$

所以：

$$
\tilde{q}_i^T\tilde{k}_j = q_i^T R_{j-i}k_j
$$

这说明 RoPE 使注意力分数与相对位置 $j-i$ 相关。

### 6. 在多头注意力中的使用

在 MHA 或 GQA 中，`RoPE` 对每个 head 的 $Q$ 和 $K$ 分别应用。

对于：

$$
Q \in \mathbb{R}^{B \times T \times H \times D}
$$

$$
K \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$

`RoPE` 作用在最后一维 $D$ 上，并且对每个位置 $t$ 使用对应的位置角度 $\alpha_{t,i}$。

`RoPE` 不改变张量形状：

$$
\tilde{Q} \in \mathbb{R}^{B \times T \times H \times D}
$$

$$
\tilde{K} \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$
