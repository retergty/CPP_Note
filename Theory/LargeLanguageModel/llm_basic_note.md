# LLM 基础知识

## 符号约定

除非特别说明，本文采用以下约定：

- $B$：batch size。
- $T$：sequence length。
- $C$：hidden size。
- $H$：query head 数量。
- $D$：单个 attention head 的维度。
- $V_{\text{vocab}}$：词表大小。
- 大写字母表示矩阵或张量，小写字母表示单个 token 的向量。
- 单个 token 的向量统一写成行向量。例如 $q_t,k_t\in\mathbb{R}^{1\times d_k}$，$v_t\in\mathbb{R}^{1\times d_v}$。
- $Q,K,V$ 按行堆叠各 token 的向量。例如 $Q=[q_1;\cdots;q_T]\in\mathbb{R}^{T\times d_k}$。
- 讨论单个样本或单个 head 时，省略 batch 和 head 下标。

## 词嵌入 Token Embedding

tokenizer 将文本转换为 token id：

$$
x\in\mathbb{Z}^{B\times T}
$$

设词表大小为 $V_{\text{vocab}}$，embedding table 为：

$$
E\in\mathbb{R}^{V_{\text{vocab}}\times C}
$$

词嵌入通过查表得到：

$$
X_{b,t}=E[x_{b,t}],
\qquad
X\in\mathbb{R}^{B\times T\times C}
$$

词嵌入只编码 token 身份。使用绝对位置编码时，通常计算：

$$
X_0=TokenEmbedding(x)+PositionEmbedding(pos)
$$

使用 `RoPE` 时，位置信息在 attention 中注入 $Q$ 和 $K$，不直接加到 token embedding 上。

## RMSNorm

`RMSNorm` 使用均方根缩放单个 token 的隐藏向量。对
$x\in\mathbb{R}^{1\times C}$：

$$
rms(x)=\sqrt{\frac{1}{C}\sum_{i=1}^{C}x_i^2+\epsilon}
$$

$$
RMSNorm(x)=\frac{x}{rms(x)}\odot g
$$

其中 $g\in\mathbb{R}^{1\times C}$ 是可学习缩放参数，$\epsilon$ 用于避免除零。对于输入 $X\in\mathbb{R}^{B\times T\times C}$，每个 token 都沿最后一维独立归一化。

## LayerNorm

`LayerNorm` 先中心化，再按标准差缩放。对
$x\in\mathbb{R}^{1\times C}$：

$$
\mu(x)=\frac{1}{C}\sum_{i=1}^{C}x_i,
\qquad
\sigma^2(x)=\frac{1}{C}\sum_{i=1}^{C}(x_i-\mu(x))^2
$$

$$
LayerNorm(x)=
\frac{x-\mu(x)}{\sqrt{\sigma^2(x)+\epsilon}}\odot g+b
$$

其中 $g,b\in\mathbb{R}^{1\times C}$ 是可学习参数。`LayerNorm` 只使用当前 token 的隐藏向量，不依赖其他 token 或 batch 内其他样本。

### 在 Transformer 中的作用

归一化用于稳定激活值和梯度。常见结构为：

- `Post-Norm`：子层计算后执行残差连接和归一化。
- `Pre-Norm`：先归一化，再执行子层和残差连接。

现代 LLM 通常采用更易训练的 `Pre-Norm`，并常用 `RMSNorm` 替代 `LayerNorm`。

## RMSNorm 和 LayerNorm 的区别

`LayerNorm` 同时移除均值并缩放方差；`RMSNorm` 不移除均值，只按均方根缩放。后者计算更简单，已用于 `LLaMA` 等模型。

## 自注意力机制 self-attention

自注意力根据 token 之间的相关性聚合序列信息。以下省略 batch 维度，设：

$$
X\in\mathbb{R}^{T\times C}
$$

### 生成 Q、K、V

通过三个线性变换生成 query、key 和 value：

$$
Q=XW_Q,\qquad K=XW_K,\qquad V=XW_V
$$

$$
W_Q,W_K\in\mathbb{R}^{C\times d_k},
\qquad
W_V\in\mathbb{R}^{C\times d_v}
$$

$$
Q,K\in\mathbb{R}^{T\times d_k},
\qquad
V\in\mathbb{R}^{T\times d_v}
$$

其中 $Q$、$K$、$V$ 的第 $t$ 行分别是 $q_t$、$k_t$、$v_t$。

### 计算注意力分数

query 与所有 key 做缩放点积：

$$
S=\frac{QK^T}{\sqrt{d_k}}
\in\mathbb{R}^{T\times T}
$$

$$
S_{i,j}=\frac{q_ik_j^T}{\sqrt{d_k}}
$$

除以 $\sqrt{d_k}$ 可避免点积幅度随维度增大而过大。

### 计算注意力权重和输出

对 $S$ 的每一行执行 `softmax`：

$$
A=softmax(S)
$$

$$
A_{i,j}=\frac{e^{S_{i,j}}}{\sum_{r=1}^{T}e^{S_{i,r}}},
\qquad
\sum_{j=1}^{T}A_{i,j}=1
$$

最后聚合 value：

$$
O=AV
=softmax\left(\frac{QK^T}{\sqrt{d_k}}\right)V
\in\mathbb{R}^{T\times d_v}
$$

## GQA 多头注意力机制 Grouped Query Attention

多头注意力在不同子空间中并行建模 token 关系。`GQA` 让一组 query head 共享同一个 key/value head，以减少参数量和 `KV Cache`。

$$
D=\frac{C}{H},
\qquad
G=\frac{H}{H_{kv}}
$$

其中 $H$ 是 query head 数量，$H_{kv}$ 是 key/value head 数量，$G$ 是每组包含的 query head 数量，要求 $H$ 能被 $H_{kv}$ 整除。

### 生成 Q、K、V

对 $X\in\mathbb{R}^{B\times T\times C}$ 线性映射并 reshape：

$$
Q\in\mathbb{R}^{B\times T\times H\times D}
$$

$$
K,V\in\mathbb{R}^{B\times T\times H_{kv}\times D}
$$

### Query head 到 Key/Value head 的分组映射

第 $h$ 个 query head 使用的 key/value head 为：

$$
m(h)=\left\lfloor\frac{h-1}{G}\right\rfloor+1,
\qquad
h\in\{1,\ldots,H\}
$$

### 每个 Query head 计算注意力

对第 $b$ 个样本、第 $h$ 个 query head：

$$
O_{b,h}
=softmax\left(
\frac{Q_{b,h}K_{b,m(h)}^T}{\sqrt D}
\right)V_{b,m(h)}
$$

$$
O_{b,h}\in\mathbb{R}^{T\times D}
$$

### 拼接所有 Query head 的输出

拼接全部 query head，再执行输出投影：

$$
O=concat(O_1,\ldots,O_H)\in\mathbb{R}^{B\times T\times C}
$$

$$
Y=OW_O,
\qquad
W_O\in\mathbb{R}^{C\times C}
$$

$$
Y\in\mathbb{R}^{B\times T\times C}
$$

### GQA 和 MHA、MQA 的关系

- 当 $H_{kv} = H$ 时，每个 query head 都有独立的 key/value head，此时为普通 `Multi-Head Attention`。
- 当 $H_{kv} = 1$ 时，所有 query head 共享同一个 key/value head，此时为 `Multi-Query Attention`。
- 当 $1 < H_{kv} < H$ 时，多个 query head 按组共享 key/value head，此时为 `Grouped Query Attention`。

## 因果注意力 Causal Attention

`Causal Attention` 保证第 $i$ 个 token 只能使用位置 $1$ 到 $i$ 的信息。缩放注意力分数为：

$$
S=\frac{QK^T}{\sqrt{d_k}}
\in\mathbb{R}^{T\times T}
$$

### 因果 Mask

定义：

$$
M_{i,j} =
\begin{cases}
0, & j \le i \\
-\infty, & j > i
\end{cases}
$$

### Masked Softmax

$$
A=softmax(S+M)
$$

当 $j>i$ 时，$A_{i,j}=0$；第 $i$ 行的有效权重仅分布在位置 $1$ 到 $i$。

### 输出

$$
O=AV,
\qquad
o_i=\sum_{j=1}^{i}A_{i,j}v_j
$$

$$
CausalAttention(Q,K,V)
=softmax\left(\frac{QK^T}{\sqrt{d_k}}+M\right)V
$$

推理时可缓存历史 $K_{\le i}$ 和 $V_{\le i}$，避免重复计算。

### 在多头注意力中的 Mask 形状

在 MHA 或 GQA 中，$S\in\mathbb{R}^{B\times H\times T\times T}$。实现时通常将 mask 组织为 $M\in\mathbb{R}^{1\times1\times T\times T}$，并广播到 batch 和 head 维度。

### Sliding Window Attention

`Sliding Window Attention` 只保留最近 $w$ 个位置：

$$
M_{i,j} =
\begin{cases}
0, & j \le i \text{ 且 } i - j < w \\
-\infty, & \text{otherwise}
\end{cases}
$$

$$
o_i=Attention\left(
q_i,
K_{\max(1,i-w+1):i},
V_{\max(1,i-w+1):i}
\right)
$$

它降低计算量和 `KV Cache` 占用，但无法直接访问窗口外的信息。

## 旋转位置编码 Rotary Position Embedding

基础的自注意力只根据 token 内容计算相关性，如果不加入位置信息，模型无法区分相同 token 出现在不同位置时的差别。

$$
S = QK^T
$$

`RoPE` 的核心思想是：根据 token 的位置，对 `Query` 和 `Key` 做旋转变换，让注意力点积自然带上位置信息。

### 为什么作用在 Q 和 K 上

注意力分数由 $Q$ 和 $K$ 的点积决定：

$$
S_{t,s} = q_tk_s^T
$$

其中 $t$ 和 $s$ 表示两个 token 的位置。

位置编码要影响注意力分数，因此 `RoPE` 作用在 $Q$ 和 $K$ 上。$V$ 只提供被聚合的内容，不直接参与注意力分数计算，因此不应用 `RoPE`。

### 输入形式

对于单个 attention head，设：

$$
Q, K \in \mathbb{R}^{T \times D}
$$

其中：

- $T$ 表示序列长度。
- $D$ 表示每个 head 的维度。
- $D$ 为偶数。

第 $t$ 个 token 的 query 和 key 是 $Q$、$K$ 的第 $t$ 行：

$$
q_t, k_t \in \mathbb{R}^{1 \times D}
$$

`RoPE` 会把最后一维 $D$ 按两维一组划分，并根据位置 $t$ 对每组维度使用不同的旋转角度。

### 旋转角度

将向量维度按两维一组划分。第 $m$ 组维度为：

$$
(2m, 2m + 1)
$$

其中：

$$
m = 0, 1, \cdots, \frac{D}{2} - 1
$$

第 $m$ 组对应的频率为：

$$
\omega_m = \theta^{-\frac{2m}{D}}
$$

第 $t$ 个位置在第 $m$ 组维度上的旋转角度为：

$$
\alpha_{t,m} = t \cdot \omega_m
$$

其中 $\theta$ 是 `RoPE` 的 base 参数，常见取值为 $10000$。不同维度组使用不同频率，用来表达不同尺度的位置信息。

### 对 Q 和 K 应用 RoPE

对每个位置 $t$，分别旋转 $q_t$ 和 $k_t$：

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

注意力分数改为使用旋转后的 $Q$ 和 $K$：

$$
S = \frac{\tilde{Q}\tilde{K}^T}{\sqrt{D}}
$$

元素级形式为：

$$
S_{t,s} = \frac{\tilde{q}_t\tilde{k}_s^T}{\sqrt{D}}
$$

其中 $\tilde{q}_t$ 带有第 $t$ 个位置的旋转信息，$\tilde{k}_s$ 带有第 $s$ 个位置的旋转信息。

### RoPE 的相对位置性质

设 $R_t$ 表示第 $t$ 个位置对应的旋转矩阵：

$$
\tilde{q}_t = q_tR_t^T
$$

$$
\tilde{k}_s = k_sR_s^T
$$

则注意力点积为：

$$
\tilde{q}_t\tilde{k}_s^T = q_tR_t^TR_sk_s^T
$$

因为旋转矩阵满足：

$$
R_t^T R_s = R_{s-t}
$$

所以：

$$
\tilde{q}_t\tilde{k}_s^T = q_tR_{s-t}k_s^T
$$

这说明位置信息会以相对位置 $s-t$ 的形式进入注意力点积。注意力分数仍然依赖 $q_t$ 和 $k_s$ 的内容，并不是只由相对位置决定。

### 在多头注意力中的使用

在 MHA 或 GQA 中，`RoPE` 对每个 head 的 $Q$ 和 $K$ 分别应用。

对于：

$$
Q \in \mathbb{R}^{B \times T \times H \times D}
$$

$$
K \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$

`RoPE` 作用在最后一维 $D$ 上，并且对每个位置 $t$ 使用对应的位置角度 $\alpha_{t,m}$。

`RoPE` 不改变张量形状：

$$
\tilde{Q} \in \mathbb{R}^{B \times T \times H \times D}
$$

$$
\tilde{K} \in \mathbb{R}^{B \times T \times H_{kv} \times D}
$$

## 推理时的注意力计算

训练时通常一次性输入长度为 $T$ 的序列，直接计算完整注意力矩阵：

$$
S = \frac{QK^T}{\sqrt{d_k}}
$$

在自回归推理时，模型是逐 token 生成的，因此注意力计算通常分为两个阶段。

### Prefill 阶段

`Prefill` 阶段处理用户输入的 prompt。假设 prompt 长度为 $T$，这一阶段仍然会计算整段 prompt 的因果注意力：

$$
O = softmax(\frac{QK^T}{\sqrt{d_k}} + M)V
$$

同时会把 prompt 中每个 token 对应的 `Key/Value` 保存到 `KV Cache` 中：

$$
K_{\text{cache}}=[k_1;\,k_2;\,\cdots;\,k_T]
\in\mathbb{R}^{T\times d_k}
$$

$$
V_{\text{cache}}=[v_1;\,v_2;\,\cdots;\,v_T]
\in\mathbb{R}^{T\times d_v}
$$

### Decode 阶段

`Decode` 阶段每次只生成一个新 token。假设当前处理第 $i$ 个位置，只需要计算当前 token 的：

$$
q_i,\ k_i,\ v_i
$$

然后把新的 $k_i$ 和 $v_i$ 追加到 `KV Cache`：

$$
K_{\text{cache}}=[k_1;\,k_2;\,\cdots;\,k_i]
$$

$$
V_{\text{cache}}=[v_1;\,v_2;\,\cdots;\,v_i]
$$

默认情况下，当前 token 的注意力会使用当前 query 和缓存中的所有 `Key/Value`：

$$
S_i = \frac{q_iK_{\text{cache}}^T}{\sqrt{d_k}}
$$

$$
A_i = softmax(S_i)
$$

$$
O_i = A_iV_{\text{cache}}
$$

因此推理时的单步注意力可以理解为：

$$
O_i = Attention(q_i, K_{\text{cache}}, V_{\text{cache}})
$$

这等价于完整因果注意力矩阵中的第 $i$ 行。区别是推理时不会重复计算历史 token 的 $K$ 和 $V$，而是直接复用缓存。

如果模型使用 `Sliding Window Attention`，推理时通常只使用最近窗口内的 `KV Cache`：

$$
O_i = Attention(q_i, K_{\max(1, i-w+1):i}, V_{\max(1, i-w+1):i})
$$

其中 $w$ 表示 `window_size`。

### 和训练时的区别

- 训练时：一次性计算完整的 $QK^T$，得到 $T \times T$ 注意力矩阵，并使用 causal mask 屏蔽未来位置。
- 推理时：每一步只计算当前 token 的一行注意力，历史 token 的 `Key/Value` 来自 `KV Cache`。
- 如果模型使用 `window_size`，训练和推理都会遵守相同的窗口限制。
- `KV Cache` 节省的是历史 $K$ 和 $V$ 的重复计算；如果没有窗口限制，当前 token 仍然需要和全部缓存位置做 attention。

## Gated DeltaNet 线性注意力

`Gated DeltaNet` 用固定大小的记忆矩阵压缩历史信息，不构造 $T\times T$ 注意力矩阵。它结合两种机制：

- `decay gate`：整体衰减旧状态。
- `delta rule`：定向修改与当前 key 相关的记忆。

以下讨论单个 head。设：

$$
q_t,k_t\in\mathbb{R}^{1\times d_k},
\qquad
v_t\in\mathbb{R}^{1\times d_v}
$$

记忆矩阵为：

$$
M_t\in\mathbb{R}^{d_k\times d_v}
$$

### 基础线性注意力

当前 key-value 通过外积写入记忆，再由 query 读取：

$$
M_t=M_{t-1}+k_t^Tv_t,
\qquad
o_t=q_tM_t
$$

展开可得：

$$
o_t=\sum_{i=1}^{t}(q_tk_i^T)v_i
$$

因此，线性注意力可以先累计 $k_i^Tv_i$，再与 $q_t$ 相乘，避免显式保存全部历史 `Key/Value`。固定大小的记忆也会带来容量限制：相似 key 写入的信息可能相互干扰。

### Delta Rule

`DeltaNet` 先读取 $k_t$ 对应的旧 value，再写入预测误差：

$$
\hat v_t=k_tM_{t-1}
$$

$$
M_t=M_{t-1}+\beta_tk_t^T(v_t-\hat v_t),
\qquad
\beta_t\in(0,1)
$$

其中 $\beta_t$ 控制写入强度。等价形式为：

$$
M_t=(I-\beta_tk_t^Tk_t)M_{t-1}+\beta_tk_t^Tv_t
$$

其中 $I\in\mathbb{R}^{d_k\times d_k}$。第一项削弱 $k_t$ 方向上的旧关联，第二项写入新的 $k_t\rightarrow v_t$ 关联。为稳定更新，通常对 query 和 key 做 L2 归一化：

$$
q_t\leftarrow\frac{q_t}{\|q_t\|_2},
\qquad
k_t\leftarrow\frac{k_t}{\|k_t\|_2}
$$

### Gated Delta Rule

`Gated DeltaNet` 先使用衰减门保留部分旧状态：

$$
\tilde M_{t-1}=\alpha_tM_{t-1},
\qquad
\alpha_t\in(0,1)
$$

再对衰减后的状态执行 `delta rule`：

$$
M_t=\tilde M_{t-1}
+\beta_tk_t^T(v_t-k_t\tilde M_{t-1})
$$

$$
o_t=q_tM_t
$$

合并后得到：

$$
M_t=
\alpha_t(I-\beta_tk_t^Tk_t)M_{t-1}
+\beta_tk_t^Tv_t
$$

两个门控制不同粒度的更新：

- $\alpha_t\rightarrow0$：快速清除大部分历史状态。
- $\alpha_t\rightarrow1$：退化为普通 `DeltaNet`。
- $\beta_t\rightarrow0$：当前 key-value 几乎不写入状态。
- $\beta_t\rightarrow1$ 且 $\|k_t\|_2=1$：用新 value 替换当前 key 方向上的旧关联。

### 复杂度

单个 head 的时间和状态空间复杂度分别为：

$$
O(Td_kd_v),
\qquad
O(d_kd_v)
$$

状态空间不随 $T$ 增长。训练时可使用 `chunkwise parallel algorithm` 提高并行度；decode 时只需更新固定大小的 $M_t$，无需维护随上下文增长的 `KV Cache`。

### 和 Softmax Attention 的区别

- `Softmax Attention` 能直接访问每个历史 token，但计算量和 `KV Cache` 随序列长度增长。
- `Gated DeltaNet` 使用固定大小的状态，长序列推理更节省显存，但可能发生记忆冲突。
- 两者可以组成混合模型：`Gated DeltaNet` 压缩长期信息，`Sliding Window Attention` 建模局部依赖。

## Transformer 结构

大语言模型通常使用 `Decoder-only Transformer` 结构。整体流程可以理解为：

```text
Token IDs -> Token Embedding -> N 个 Transformer Block -> LM Head -> logits
```

给定输入 token 序列：

$$
x \in \mathbb{Z}^{B \times T}
$$

经过词嵌入后得到：

$$
X_0 \in \mathbb{R}^{B \times T \times C}
$$

### Transformer Block

一个 decoder-only 的 `Transformer Block` 通常由两部分组成：

- 自注意力层 `Self-Attention`
- 前馈网络 `Feed Forward Network`

常见的大模型一般使用 `Pre-Norm` 结构，即先做归一化，再进入子层：

$$
A_l = X_l + Attention(Norm(X_l))
$$

$$
X_{l+1} = A_l + FFN(Norm(A_l))
$$

其中 $l$ 表示第 $l$ 层。

残差连接可以保留原始信息，归一化可以稳定训练。

### Self-Attention 层

在 LLM 中，`Self-Attention` 通常会同时使用：

- `Causal Attention`：保证当前位置不能看到未来 token。
- `RoPE`：把位置信息注入到 $Q$ 和 $K$。
- `MHA` 或 `GQA`：使用多个 attention head 表达不同子空间的信息。

因此 attention 层可以简化理解为：

$$
Attention(X) = CausalAttention(Q, K, V)
$$

其中 $Q$、$K$、$V$ 都由当前层输入 $X$ 线性映射得到。

### FFN 层

`FFN` 对每个 token 位置独立计算，用来增强非线性表达能力。它不会在不同 token 之间交换信息，不改变序列长度：

$$
FFN: \mathbb{R}^{B \times T \times C} \rightarrow \mathbb{R}^{B \times T \times C}
$$

注意力层负责 token 之间的信息交互，`FFN` 层负责对每个 token 的表示做进一步变换。

设输入包含$N$个`Token`,每个`Token`的维度为$d$

$$
X \in \mathbb{R}^{d \times N}
$$

#### 通用FFN

$$
Y = W_{down} \phi(W_{up}X+b_{up}) + b_{down}
$$

其中各矩阵/向量维度为：

| 符号 | 维度 | 含义 |
| ------ | ------ | ------ |
| $X$ | $\mathbb{R}^{d \times N}$ | 输入，$N$ 个 token，每个维度 $d$ |
| $W_{up}$ | $\mathbb{R}^{m \times d}$ | 升维投影，$d \to m$（$m$ 为中间层宽度，常取 $4d$） |
| $b_{up}$ | $\mathbb{R}^{m}$ | 升维偏置，对 $N$ 个位置广播 |
| $\phi(\cdot)$ | $\mathbb{R} \to \mathbb{R}$（逐元素） | 激活函数，作用于中间表示 |
| $W_{down}$ | $\mathbb{R}^{d \times m}$ | 降维投影，$m \to d$ |
| $b_{down}$ | $\mathbb{R}^{d}$ | 降维偏置，对 $N$ 个位置广播 |
| $Y$ | $\mathbb{R}^{d \times N}$ | 输出，与输入同形状 |

#### Gate FFN

$$
Y = W_{down}[\phi(W_{gate}X+b_{gate}) \odot (W_{up}X+b_{up})] + b_{down}
$$

相对通用 FFN，多了一条门控支路：$W_{gate}$ 经激活后与 $W_{up}$ 的结果做逐元素相乘，再经 $W_{down}$ 投影回 $d$ 维（如 SwiGLU）。

其中各矩阵/向量维度为：

| 符号 | 维度 | 含义 |
| ------ | ------ | ------ |
| $X$ | $\mathbb{R}^{d \times N}$ | 输入，$N$ 个 token，每个维度 $d$ |
| $W_{gate}$ | $\mathbb{R}^{m \times d}$ | 门控投影，$d \to m$ |
| $b_{gate}$ | $\mathbb{R}^{m}$ | 门控偏置，对 $N$ 个位置广播 |
| $W_{up}$ | $\mathbb{R}^{m \times d}$ | 升维投影，$d \to m$ |
| $b_{up}$ | $\mathbb{R}^{m}$ | 升维偏置，对 $N$ 个位置广播 |
| $\phi(\cdot)$ | $\mathbb{R} \to \mathbb{R}$（逐元素） | 激活函数，只作用于门控支路 |
| $\phi(W_{gate}X+b_{gate})$ | $\mathbb{R}^{m \times N}$ | 门控信号 |
| $W_{up}X+b_{up}$ | $\mathbb{R}^{m \times N}$ | 升维后的值 |
| $\odot$ | 同形逐元素乘 | 门控结果，形状仍为 $\mathbb{R}^{m \times N}$ |
| $W_{down}$ | $\mathbb{R}^{d \times m}$ | 降维投影，$m \to d$ |
| $b_{down}$ | $\mathbb{R}^{d}$ | 降维偏置，对 $N$ 个位置广播 |
| $Y$ | $\mathbb{R}^{d \times N}$ | 输出，与输入同形状 |

即：两条 $d\to m$ 支路在中间层用 $\odot$ 融合后，再由 $W_{down}$ 压回 $d$，序列长度 $N$ 不变。

### 输出层

经过 $N$ 层 `Transformer Block` 后，得到最终隐藏状态：

$$
X_N \in \mathbb{R}^{B \times T \times C}
$$

最后通过 `LM Head` 映射到词表空间：

$$
logits=LMHead(X_N)
$$

## LM Head

`LM Head` 是语言模型最后的输出层，用来把隐藏状态映射到词表空间。

经过 $N$ 层 `Transformer Block` 后，隐藏状态为：

$$
X_N \in \mathbb{R}^{B \times T \times C}
$$

设词表大小为 $V_{\text{vocab}}$，`LM Head` 的权重为：

$$
W_{vocab}\in\mathbb{R}^{C\times V_{\text{vocab}}}
$$

$$
logits=X_NW_{vocab}
\in\mathbb{R}^{B\times T\times V_{\text{vocab}}}
$$

其中 $logits_{b,t,:}$ 是位置 $(b,t)$ 对整个词表的原始预测分数。

### 训练时

训练自回归语言模型时，第 $t$ 个位置的输出用来预测第 $t+1$ 个 token。

$$
logits_{b,t,:}\rightarrow x_{b,t+1}
$$

`logits` 不是概率，也不需要预先执行 `softmax`。

### 构造 targets

目标 token 就是输入序列右移一位：

$$
targets_{b,t} = x_{b,t+1}
$$

因此实际参与损失计算的张量为：

$$
shift\_logits
=logits[:, :-1, :]
\in\mathbb{R}^{B\times(T-1)\times V_{\text{vocab}}}
$$

$$
targets = x[:, 1:] \in \mathbb{Z}^{B \times (T-1)}
$$

例如输入为：

```text
[BOS, 我, 喜欢, 学习]
```

则各位置的预测目标为：

```text
BOS   -> 我
我    -> 喜欢
喜欢  -> 学习
```

最后一个位置没有下一个 token，因此通常不参与本段序列的 loss。

`targets` 中的元素是正确 token 的 id，取值范围为 $[0,V_{\text{vocab}}-1]$，无需转换成 one-hot 向量。

### 单个位置的交叉熵

交叉熵会在词表维度上计算 `softmax`，并取正确 target 对应概率的负对数：

$$
\ell_{b,t} =
-\log \left(softmax(logits_{b,t,:})_{targets_{b,t}}\right)
$$

模型给正确 token 分配的概率越高，$\ell_{b,t}$ 越小。

### 整个 batch 的 loss

对所有有效 token 位置的损失取平均：

$$
Loss =
\frac{1}{N}\sum_{(b,t) \in \mathcal{I}} \ell_{b,t}
$$

其中 $\mathcal{I}$ 表示有效位置集合，$N$ 是有效位置数。padding、prompt 中不需要训练的位置等通常会标记为 `ignore_index`，不参与 loss。

### 推理时

推理生成时，通常只取最后一个位置的 logits：

$$
logits_{last}\in\mathbb{R}^{B\times V_{\text{vocab}}}
$$

然后根据采样策略选择下一个 token，例如 `argmax`、`top-k`、`top-p` 或 `temperature sampling`。

### 权重共享

有些模型会让 `LM Head` 和输入端的 token embedding 共享权重：

$$
W_{vocab} = E^T
$$

这样可以减少参数量，也让输入 token 表示和输出词表分类使用同一套语义空间。

## 监督微调 Supervised Fine-Tuning

`SFT` 使用高质量的指令与回答数据继续训练预训练模型，使模型学会理解指令并按照期望的格式回答。

### 训练数据

一条 SFT 数据通常包含：

```text
System: 你是一个有帮助的助手。
User: 请解释什么是自注意力。
Assistant: 自注意力是一种……
```

经过 chat template 和 tokenizer 处理后，整段对话会被拼接成一个 token 序列：

$$
x = [x_1, x_2, \cdots, x_T]
$$

模型仍然使用自回归方式，根据前面的 token 预测下一个 token。

### Targets 和 Loss Mask

targets 仍然由输入序列右移一位得到：

$$
targets_t = x_{t+1}
$$

但 SFT 通常只计算 `Assistant` 回答部分的 loss，`System`、`User` 和 padding 位置不参与损失计算：

$$
m_t =
\begin{cases}
1, & targets_t \text{ 属于 Assistant 回答} \\
0, & \text{otherwise}
\end{cases}
$$

最终损失为：

$$
Loss =
\frac{\sum_t m_t \ell_t}
{\sum_t m_t}
$$

其中 $\ell_t$ 是第 $t$ 个位置的 token 交叉熵损失。

只计算回答部分的 loss，可以让模型利用完整对话作为上下文，同时主要学习如何生成期望的回答。

### Teacher Forcing

训练时，模型每个位置接收到的都是数据中的真实历史 token，而不是模型自己生成的 token。这种方式称为 `Teacher Forcing`。

因此一条长度为 $T$ 的样本可以并行计算所有有效位置的 next-token loss，而不需要像推理时一样逐 token 生成。

### 多轮对话

对于多轮对话，可以只训练最后一轮回答，也可以训练所有 `Assistant` 回答：

```text
System  -> 不计算 loss
User    -> 不计算 loss
Assistant -> 计算 loss
User    -> 不计算 loss
Assistant -> 计算 loss
```

具体哪些位置参与 loss 由训练数据的 mask 策略决定。

### 和预训练的区别

- 预训练主要使用大规模文本学习语言知识和 next-token prediction。
- SFT 使用规模更小、质量更高的指令回答数据，学习指令遵循和回答格式。
- 两者通常使用相同的自回归交叉熵目标，主要区别在于训练数据和参与 loss 的 token 范围。
