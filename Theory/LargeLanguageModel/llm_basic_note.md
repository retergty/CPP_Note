# LLM 基础知识

## 符号约定

除非特别说明，本文采用以下约定：

- $B$：batch size。
- $T$：sequence length。
- $C$：hidden size。
- $H$：query head 数量。
- $H_{kv}$：key/value head 数量。
- $D$：单个 attention head 的维度。
- $d_k,d_v$：省略多头结构时，query/key 和 value 的特征维度。
- $C_{ff}$：FFN 中间层宽度。
- $V_{\text{vocab}}$：词表大小。
- 大写字母表示矩阵或张量，小写字母表示单个 token 的向量。
- 单个 token 的向量统一写成行向量。例如 $q_t,k_t\in\mathbb{R}^{1\times d_k}$，$v_t\in\mathbb{R}^{1\times d_v}$。
- $Q,K,V$ 按行堆叠各 token 的向量。例如 $Q=[q_1;\cdots;q_T]\in\mathbb{R}^{T\times d_k}$。
- 讨论单个样本或单个 head 时，省略 batch 和 head 下标。

## Decoder-only Transformer 总览

大语言模型通常采用`Decoder-only Transformer`。从输入到输出的主流程为：

```text
Token IDs
    ↓
Token Embedding
    ↓
N 个 Transformer Block
    ↓
Final Norm
    ↓
LM Head
    ↓
logits
```

设第$l$层的输入为$X_l$，现代 LLM 常用的`Pre-Norm`结构可以写为：

$$
A_l
=
X_l+\operatorname{Attention}
\left(
\operatorname{Norm}(X_l)
\right)
$$

$$
X_{l+1}
=
A_l+\operatorname{FFN}
\left(
\operatorname{Norm}(A_l)
\right)
$$

每个`Transformer Block`包含两条主要计算路径：

- `Self-Attention`负责不同 token 之间的信息交互。
- `FFN`对每个 token 的表示独立进行非线性变换。

两条路径都使用残差连接保留输入信息，并使用归一化稳定训练。后续章节按照数据流依次介绍这些组件。

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

## 归一化与残差连接

归一化用于控制隐藏状态的数值尺度，残差连接用于保留原始信息并改善深层网络中的梯度传播。归一化只沿单个 token 的隐藏维度计算，不会混合不同 token 或不同样本的信息。

### RMSNorm

`RMSNorm` 使用均方根缩放单个 token 的隐藏向量。对
$x\in\mathbb{R}^{1\times C}$：

$$
rms(x)=\sqrt{\frac{1}{C}\sum_{i=1}^{C}x_i^2+\epsilon}
$$

$$
RMSNorm(x)=\frac{x}{rms(x)}\odot g
$$

其中 $g\in\mathbb{R}^{1\times C}$ 是可学习缩放参数，$\epsilon$ 用于避免除零。对于输入 $X\in\mathbb{R}^{B\times T\times C}$，每个 token 都沿最后一维独立归一化。

### LayerNorm

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

### Pre-Norm 与 Post-Norm

归一化用于稳定激活值和梯度。常见结构为：

- `Post-Norm`：子层计算后执行残差连接和归一化。
- `Pre-Norm`：先归一化，再执行子层和残差连接。

现代 LLM 通常采用更易训练的 `Pre-Norm`，并常用 `RMSNorm` 替代 `LayerNorm`。

### RMSNorm 和 LayerNorm 的区别

`LayerNorm` 同时移除均值并缩放方差；`RMSNorm` 不移除均值，只按均方根缩放。后者计算更简单，已用于 `LLaMA` 等模型。

## 自注意力机制 Self-Attention

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

## 多头注意力及其变体 MHA、GQA、MQA

单头注意力只在一个表示子空间中计算 token 关系。多头注意力把隐藏维度拆成多个 head，使不同 head 可以并行关注不同类型的关系。

设 query head 数量为$H$，单个 head 的维度为：

$$
D=\frac{C}{H}
$$

### Multi-Head Attention

对于输入：

$$
X\in\mathbb{R}^{B\times T\times C}
$$

经过线性投影和 reshape 后得到：

$$
Q,K,V
\in
\mathbb{R}^{B\times T\times H\times D}
$$

第$h$个 head 独立计算：

$$
O_h
=
\operatorname{softmax}
\left(
\frac{Q_hK_h^T}{\sqrt D}
\right)V_h
$$

$$
O_h\in\mathbb{R}^{B\times T\times D}
$$

所有 head 的输出沿隐藏维度拼接，再执行输出投影：

$$
O
=
\operatorname{Concat}(O_0,\ldots,O_{H-1})
\in
\mathbb{R}^{B\times T\times C}
$$

$$
Y=OW_O,
\qquad
W_O\in\mathbb{R}^{C\times C}
$$

### Grouped Query Attention

`MHA`为每个 query head 分配独立的 key/value head。`GQA`让一组 query head 共享同一个 key/value head，以减少参数量和`KV Cache`占用。

设 key/value head 数量为$H_{kv}$，每组 query head 数量为：

$$
G=\frac{H}{H_{kv}}
$$

要求$H$能被$H_{kv}$整除。此时：

$$
Q\in\mathbb{R}^{B\times T\times H\times D}
$$

$$
K,V\in\mathbb{R}^{B\times T\times H_{kv}\times D}
$$

使用从$0$开始的 head 下标时，第$h$个 query head 对应的 key/value head 为：

$$
m(h)=\left\lfloor\frac{h}{G}\right\rfloor,
\qquad
0\le h<H
$$

因此：

$$
O_h
=
\operatorname{softmax}
\left(
\frac{Q_hK_{m(h)}^T}{\sqrt D}
\right)V_{m(h)}
$$

输出仍然拼接为$B\times T\times C$，所以`GQA`不会改变注意力层的最终输出形状。

### 三种结构的关系

- $H_{kv}=H$：每个 query head 使用独立的 key/value head，即`MHA`。
- $1<H_{kv}<H$：多个 query head 分组共享 key/value head，即`GQA`。
- $H_{kv}=1$：所有 query head 共享一个 key/value head，即`MQA`。

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

## 位置编码扩展：M-RoPE

`M-RoPE`是`Multimodal Rotary Position Embedding`的缩写。它把普通`RoPE`的一维位置扩展为时间、高度和宽度三个坐标，使语言模型能够在同一序列中统一表示文本、图像和视频的位置。

`M-RoPE`通常作用于多模态语言模型中的$Q$和$K$。它与视觉编码器内部使用的位置编码不是同一个阶段。

### 从一维位置扩展到三维位置

普通`RoPE`为第$n$个 token 使用一个位置$p_n$。`M-RoPE`则为其分配三个位置坐标：

$$
\boldsymbol{p}_n=
\left(
p_n^{(t)},p_n^{(h)},p_n^{(w)}
\right)
$$

其中：

- $p_n^{(t)}$：时间位置。
- $p_n^{(h)}$：高度或行位置。
- $p_n^{(w)}$：宽度或列位置。

包含 batch 维度时，位置索引通常可以表示为：

$$
P\in\mathbb{Z}^{B\times3\times T}
$$

具体实现也可能使用$[3,B,T]$等维度顺序。

对于第$m$组旋转维度，设其使用的坐标轴为：

$$
a(m)\in\{t,h,w\}
$$

则第$n$个 token 在该组维度上的旋转角度为：

$$
\alpha_{n,m}
=
p_n^{(a(m))}\omega_m
$$

`M-RoPE`仍然使用普通`RoPE`的二维旋转，只是不同旋转维度组可以选择不同的位置坐标。

### 不同模态的位置坐标

#### 文本

文本 token 的三个坐标相同：

$$
\boldsymbol{p}_n=(p_n,p_n,p_n)
$$

因此无论第$m$组选择哪个坐标轴，都有：

$$
\alpha_{n,m}=p_n\omega_m
$$

所以纯文本上的`M-RoPE`退化为普通的一维`RoPE`。

#### 图像

图像经过视觉编码器和空间合并后会形成二维视觉 token 网格。设该网格大小为$h\times w$，起始位置为$p_0$，第$(r,c)$个视觉 token 可以使用：

$$
\boldsymbol{p}_{r,c}
=
(p_0,p_0+r,p_0+c)
$$

其中：

$$
0\le r<h,
\qquad
0\le c<w
$$

同一张图像的时间坐标相同，而高度和宽度坐标随 token 所在的行、列变化。例如，一个从$p_0=10$开始的$2\times3$网格可以表示为：

```text
(10, 10, 10)  (10, 10, 11)  (10, 10, 12)
(10, 11, 10)  (10, 11, 11)  (10, 11, 12)
```

这保留了图像的二维空间结构，而不是只按照展平后的 token 顺序编码位置。

#### 视频

视频 token 同时使用时间、行和列坐标：

$$
\boldsymbol{p}_{f,r,c}
=
(p_0+f,p_0+r,p_0+c)
$$

其中$f$表示时间网格或帧的位置。实际模型也可以根据采样间隔或时间戳缩放时间坐标，使不同帧率的视频具有一致的时间语义。

### 多种模态之间的位置衔接

一段图像或视频结束后，后续文本通常从前一模态三个坐标的最大值加一处继续编号：

$$
p_{\text{next}}
=
\max_{n,a}p_n^{(a)}+1
$$

例如，上面的$2\times3$图像包含$6$个视觉 token，但其最大坐标是$12$，因此后续文本可以从$13$开始，而不是按照视觉 token 数量从$16$开始。

这种编号方式既保留了空间结构，也避免高分辨率图像和长视频使位置编号增长得与视觉 token 数量一样快。

### 多维相对位置性质

设第$m$组维度使用坐标轴$a(m)$。两个 token $i$和$j$在该组维度上的旋转矩阵满足：

$$
R\left(\alpha_{i,m}\right)^T
R\left(\alpha_{j,m}\right)
=
R\left(
\left(
p_j^{(a(m))}-p_i^{(a(m))}
\right)\omega_m
\right)
$$

因此：

- 时间维度组感知两个 token 的相对时间位置。
- 高度维度组感知相对行位置。
- 宽度维度组感知相对列位置。

`M-RoPE`由此把普通`RoPE`的一维相对位置性质扩展到时空坐标。

### 旋转维度的分配方式

原始的分段式`M-RoPE`把旋转维度连续划分给时间、高度和宽度：

```text
[T, T, T, ..., H, H, H, ..., W, W, W, ...]
```

由于不同旋转维度对应不同频率，这会使三个坐标轴分别集中在不同频段。

`Interleaved M-RoPE`把三个坐标轴交错分配到旋转维度：

```text
[T, H, W, T, H, W, T, H, W, ...]
```

对应的频率分配可以表示为：

```text
(T, ω0), (H, ω1), (W, ω2),
(T, ω3), (H, ω4), (W, ω5), ...
```

这样每个坐标轴都能覆盖较宽的频率范围，减轻分段式频率分配带来的频谱偏置。各模型使用的分段长度和交错规则可能不同，需要以模型配置为准。

### 与 Vision RoPE 的区别

- `Vision RoPE`用在视觉编码器内部，根据图像 patch 的二维坐标旋转视觉注意力的$Q$和$K$。
- `M-RoPE`用在多模态语言模型内部，为已经拼接到同一序列中的文本、图像和视频 token 提供统一位置坐标。
- 同一个多模态模型可以先在视觉编码器中使用`Vision RoPE`，再在语言模型中使用`M-RoPE`。

推理时，`Prefill`阶段需要为整个多模态序列构造三维位置索引；进入`Decode`阶段后，新生成的文本 token 使用相同的三维文本位置继续递增。已经写入`KV Cache`的$K$保留其对应的`M-RoPE`旋转结果。

## Softmax Attention 完整数据流

把前面的组件组合起来，一层现代`Softmax Attention`可以按以下顺序理解：

```text
输入 X
  ↓
线性投影并拆分多个 head
  ↓
生成 Q、K、V
  ↓
对 Q、K 应用 RoPE 或 M-RoPE
  ↓
计算缩放点积
  ↓
加入 Causal Mask 或 Sliding Window Mask
  ↓
Softmax 后聚合 V
  ↓
拼接所有 query head
  ↓
输出投影 W_O
```

省略 batch 和 head 下标时，核心计算为：

$$
\tilde Q,\tilde K
=
\operatorname{RoPE}(Q,K)
$$

$$
A
=
\operatorname{softmax}
\left(
\frac{\tilde Q\tilde K^T}{\sqrt D}+M
\right)
$$

$$
Y
=
\operatorname{Concat}(AV)W_O
$$

对于`GQA`，每个 query head 只使用其所属分组对应的 key/value head；其余计算顺序与`MHA`相同。

## FlashAttention

`FlashAttention`不会改变注意力的数学定义，而是通过分块计算和`Online Softmax`减少显存读写。在浮点舍入误差允许的范围内，它与标准：

$$
\operatorname{softmax}
\left(
\frac{QK^T}{\sqrt D}+M
\right)V
$$

计算相同的结果，因此它是精确的 Attention 实现，而不是稀疏注意力或近似注意力。

### 标准 Attention 的瓶颈

对单个 query $q$，设$\mu_i$是第$i$个位置的 mask bias，则：

$$
s_i
=
\frac{qk_i^T}{\sqrt D}+\mu_i
$$

为避免指数溢出，稳定的`Softmax`会减去最大值：

$$
m=\max_i s_i
$$

$$
\ell
=
\sum_i e^{s_i-m}
$$

$$
o
=
\frac{1}{\ell}
\sum_i e^{s_i-m}v_i
$$

对完整序列，标准实现通常会把分数矩阵$S=QK^T/\sqrt D+M$以及注意力概率矩阵$A=\operatorname{softmax}(S)$写入显存。二者的形状均为：

$$
S,A
\in
\mathbb{R}^{T_q\times T_{kv}}
$$

当序列很长时，主要瓶颈不仅是计算量，还包括反复读写这两个大矩阵产生的显存占用和显存带宽开销。

### Online Softmax

`FlashAttention`把$K$和$V$划分为多个块，依次处理每个块，而不保存完整的$s_i$或$\alpha_i$。对于每个 query，只维护三个运行统计量：

- $m$：已经处理过的分数最大值。
- $\ell$：以$m$为基准缩放后的指数和。
- $u$：以$m$为基准缩放后的加权 value 和。

初始化为：

$$
m^{old}=-\infty,
\qquad
\ell^{old}=0,
\qquad
u^{old}=0
$$

设新读入的 key/value 块对应索引集合$\mathcal{B}$，先计算该块的分数$s_i$，再更新最大值：

$$
m^{new}
=
\max
\left(
m^{old},
\max_{i\in\mathcal{B}}s_i
\right)
$$

新的指数和为：

$$
\ell^{new}
=
e^{m^{old}-m^{new}}\ell^{old}
+
\sum_{i\in\mathcal{B}}
e^{s_i-m^{new}}
$$

新的加权 value 和为：

$$
u^{new}
=
e^{m^{old}-m^{new}}u^{old}
+
\sum_{i\in\mathcal{B}}
e^{s_i-m^{new}}v_i
$$

如果新块产生了更大的最大值，因子$e^{m^{old}-m^{new}}$会把旧统计量重新缩放到新的指数基准。处理完所有 key/value 块后：

$$
o=\frac{u}{\ell}
$$

因此，不论 key/value 被分成多少块，最终结果都与一次性计算稳定`Softmax`一致。

### 分块计算流程

完整实现会同时对$Q$、$K$和$V$分块：

```text
从显存加载一个 Q 块到片上存储
             ↓
依次加载多个 K/V 块
             ↓
计算当前分数块
             ↓
应用 Causal/Sliding Window Mask
             ↓
使用 Online Softmax 更新 m、ℓ、u
             ↓
处理完所有 K/V 块后写回输出
```

每次只把小块放入寄存器或共享内存等片上存储，分数块使用后立即丢弃，从而避免把完整的$S$和$A$写入显存。因果注意力还可以跳过完全位于未来区域的块。

### 复杂度与适用场景

- 计算复杂度仍为$O(T_qT_{kv}D)$，FlashAttention并没有消除注意力的二次计算量。
- 不再物化$T_q\times T_{kv}$的分数和概率矩阵，中间激活显存由二次增长降为近似线性增长。
- 通过融合多个算子并减少显存读写，实际运行速度通常明显提升。
- `RoPE`或`M-RoPE`在分块计算前作用于$Q$和$K$；`MHA`、`GQA`、因果 mask 和滑动窗口均可与 FlashAttention 配合使用。
- 训练和`Prefill`阶段包含大量 query，通常最能受益；`Decode`阶段每步通常只有一个 query，常使用针对解码优化的`FlashDecoding`等内核。

`PagedAttention`主要解决`KV Cache`的分页存储和管理问题，而`FlashAttention`主要优化 Attention 的分块计算与显存访问，两者解决的问题不同，也可以同时使用。

## FlashDecoding

`FlashDecoding`是面向自回归`Decode`阶段的 Attention 优化。它沿用`FlashAttention`的分块计算与`Online Softmax`，但进一步把同一个 query 对应的长`KV Cache`切成多个区间并行处理。

### Decode 阶段的并行度问题

在`Prefill`阶段：

$$
T_q\approx T_{kv}
$$

模型可以同时处理大量 query block，因此`FlashAttention`通常具有足够的并行度。

在单步`Decode`阶段，每条序列通常只有当前 token 的一个 query：

$$
T_q=1,
\qquad
T_{kv}=T_{\text{context}}
$$

虽然 query 数量很少，但它仍然需要读取整个`KV Cache`并计算：

$$
o
=
\operatorname{softmax}
\left(
\frac{qK_{\text{cache}}^T}{\sqrt D}
\right)
V_{\text{cache}}
$$

如果只为每个 query head 启动少量计算块，长`KV Cache`会被串行遍历，GPU 可能无法得到足够的并行任务。上下文越长，该问题越明显。

### 沿 KV 序列并行切分

`FlashDecoding`把长度为$T_{kv}$的`KV Cache`划分为$R$个区间：

$$
(K,V)
=
\operatorname{Concat}_{r=1}^{R}
\left(
K^{(r)},V^{(r)}
\right)
$$

同一个 query $q$会被发送到多个计算块，各计算块并行处理不同的$K/V$区间：

```text
                       ┌-> K/V Split 1 -> 局部统计量
当前 Query q ----------+-> K/V Split 2 -> 局部统计量
                       ├-> K/V Split 3 -> 局部统计量
                       └-> K/V Split R -> 局部统计量
                                      ↓
                              合并 Online Softmax
                                      ↓
                                 Attention 输出
```

对于第$r$个区间，首先计算局部分数：

$$
s_i^{(r)}
=
\frac{q{k_i^{(r)}}^T}{\sqrt D}
+\mu_i^{(r)}
$$

其中$\mu_i^{(r)}$是 mask bias。然后得到三个局部统计量：

$$
m_r
=
\max_i s_i^{(r)}
$$

$$
\ell_r
=
\sum_i e^{s_i^{(r)}-m_r}
$$

$$
u_r
=
\sum_i e^{s_i^{(r)}-m_r}v_i^{(r)}
$$

这些区间相互独立，因此可以在 GPU 上并行计算。

### 合并局部 Softmax

不同区间使用各自的最大值$m_r$作为指数基准，不能直接把$\ell_r$和$u_r$相加。首先计算全局最大值：

$$
m=\max_r m_r
$$

再把每个区间重新缩放到全局基准：

$$
\ell
=
\sum_{r=1}^{R}
e^{m_r-m}\ell_r
$$

$$
u
=
\sum_{r=1}^{R}
e^{m_r-m}u_r
$$

最终输出为：

$$
o=\frac{u}{\ell}
$$

该合并过程与`Online Softmax`的分块更新等价，因此结果仍与完整的标准 Attention 一致。

### 性能特点

- 单 token、单个 head 的计算复杂度仍为$O(T_{kv}D)$，并没有减少需要读取和计算的 key/value 数量。
- 主要收益是把一个长 KV 序列拆成更多并行任务，提高 GPU 占用率和显存带宽利用率。
- 每个区间只输出$m_r$、$\ell_r$和$u_r$，随后通过一次归约得到最终结果，不需要物化完整注意力分数。
- 区间过少会导致并行度不足，区间过多则会增加中间统计量、归约和 kernel 调度开销，因此切分数量需要根据上下文长度和硬件选择。
- 当 batch、query head 或并发序列本身已经提供足够并行度时，额外 KV 切分的收益可能减小。

### 与其他优化的关系

- `FlashAttention`主要通过$Q/K/V$分块和`Online Softmax`减少中间矩阵的显存读写，最适合 query 较多的训练和`Prefill`阶段。
- `FlashDecoding`重点解决`Decode`阶段$T_q$很小、$T_{kv}$很大时的并行度不足。
- `PagedAttention`负责以分页方式组织不同请求的`KV Cache`，减少内存碎片并支持动态批处理。
- 实际推理框架可以同时使用分页 KV Cache、量化 KV Cache 和 FlashDecoding 风格的计算内核。
- `GQA`或`MQA`通过减少 key/value head 数降低 KV Cache 大小；FlashDecoding仍可在每个共享 key/value head 的序列维度上进行切分。
- 使用滑动窗口时，只需要切分和读取窗口范围内的 KV；窗口较短时，FlashDecoding带来的额外并行收益也会降低。

## 前馈网络 Feed Forward Network

`FFN`对每个 token 独立执行相同的非线性变换，不会在 token 之间交换信息，也不会改变 batch 大小和序列长度：

$$
\operatorname{FFN}:
\mathbb{R}^{B\times T\times C}
\rightarrow
\mathbb{R}^{B\times T\times C}
$$

注意力层负责混合不同 token 的信息，`FFN`负责变换每个 token 内部的特征。

### 普通 FFN

设中间层宽度为$C_{ff}$，使用本文约定的行向量形式：

$$
H
=
\phi(XW_{up}+b_{up})
$$

$$
Y
=
HW_{down}+b_{down}
$$

其中：

$$
W_{up}\in\mathbb{R}^{C\times C_{ff}},
\qquad
b_{up}\in\mathbb{R}^{C_{ff}}
$$

$$
W_{down}\in\mathbb{R}^{C_{ff}\times C},
\qquad
b_{down}\in\mathbb{R}^{C}
$$

因此：

$$
H\in\mathbb{R}^{B\times T\times C_{ff}},
\qquad
Y\in\mathbb{R}^{B\times T\times C}
$$

偏置项是否存在取决于具体模型。

### Gated FFN

现代 LLM 常使用带门控的 FFN。两条升维支路分别生成门控和值：

$$
G
=
\phi(XW_{gate}+b_{gate})
$$

$$
U
=
XW_{up}+b_{up}
$$

然后逐元素融合并投影回隐藏维度：

$$
Y
=
(G\odot U)W_{down}+b_{down}
$$

其中：

$$
W_{gate},W_{up}
\in
\mathbb{R}^{C\times C_{ff}},
\qquad
W_{down}
\in
\mathbb{R}^{C_{ff}\times C}
$$

当$\phi$使用`SiLU`时，该结构通常称为`SwiGLU`。门控支路控制哪些中间特征被传递，值支路提供待变换的内容。

## 输出层 LM Head

经过$N$个`Transformer Block`和最终归一化后，得到隐藏状态：

$$
X_N\in\mathbb{R}^{B\times T\times C}
$$

`LM Head`把每个 token 的隐藏状态映射到词表空间。设词表大小为$V_{\text{vocab}}$：

$$
W_{\text{vocab}}
\in
\mathbb{R}^{C\times V_{\text{vocab}}}
$$

$$
logits
=
X_NW_{\text{vocab}}
\in
\mathbb{R}^{B\times T\times V_{\text{vocab}}}
$$

$logits_{b,t,:}$是位置$(b,t)$对整个词表的原始预测分数。它不是概率；训练时的交叉熵或推理时的采样过程会在需要时计算`softmax`。

有些模型让输入端的 token embedding 和`LM Head`共享权重：

$$
W_{\text{vocab}}=E^T
$$

权重共享可以减少参数量，并让输入表示与输出词表分类使用同一套语义空间。

## 自回归训练目标

自回归语言模型使用位置$t$的输出预测位置$t+1$的 token：

$$
logits_{b,t,:}
\longrightarrow
x_{b,t+1}
$$

### Shift Logits 和 Targets

目标 token 是输入序列右移一位：

$$
shift\_logits
=
logits[:,:-1,:]
\in
\mathbb{R}^{B\times(T-1)\times V_{\text{vocab}}}
$$

$$
targets
=
x[:,1:]
\in
\mathbb{Z}^{B\times(T-1)}
$$

例如：

```text
输入： [BOS, 我, 喜欢, 学习]
目标： [我,  喜欢, 学习]

BOS  -> 我
我   -> 喜欢
喜欢 -> 学习
```

最后一个输入位置在当前序列中没有下一个 token，因此不参与该序列的 loss。`targets`保存正确 token 的 id，不需要转换成 one-hot 向量。

### 交叉熵损失

单个有效位置的交叉熵为：

$$
\ell_{b,t}
=
-\log
\left(
\operatorname{softmax}(logits_{b,t,:})_{targets_{b,t}}
\right)
$$

对所有有效 token 位置取平均：

$$
Loss
=
\frac{1}{|\mathcal{I}|}
\sum_{(b,t)\in\mathcal{I}}
\ell_{b,t}
$$

其中$\mathcal{I}$是有效位置集合。padding 或不需要训练的位置通常标记为`ignore_index`，不参与损失计算。

## 自回归推理

训练时可以并行计算整段序列的因果注意力；推理时则逐 token 生成。为了避免重复计算历史 token 的 key 和 value，自回归推理使用`KV Cache`，并分为`Prefill`和`Decode`两个阶段。

以下使用$\tilde Q$和$\tilde K$表示已经应用`RoPE`或`M-RoPE`的位置编码结果。若模型使用其他位置编码，也可以把它们理解为实际参与注意力计算的 query 和 key。

### Prefill 阶段

`Prefill` 阶段处理用户输入的 prompt。假设 prompt 长度为 $T$，这一阶段仍然会计算整段 prompt 的因果注意力：

$$
O
=
\operatorname{softmax}
\left(
\frac{\tilde Q\tilde K^T}{\sqrt{d_k}}+M
\right)V
$$

同时把已经应用位置编码的 key 和对应的 value 保存到`KV Cache`：

$$
K_{\text{cache}}
=
[\tilde k_1;\,\tilde k_2;\,\cdots;\,\tilde k_T]
\in\mathbb{R}^{T\times d_k}
$$

$$
V_{\text{cache}}=[v_1;\,v_2;\,\cdots;\,v_T]
\in\mathbb{R}^{T\times d_v}
$$

### Decode 阶段

`Decode` 阶段每次只生成一个新 token。假设当前处理第 $i$ 个位置，只需要计算当前 token 的：

$$
\tilde q_i,\ \tilde k_i,\ v_i
$$

然后把新的$\tilde k_i$和$v_i$追加到`KV Cache`：

$$
K_{\text{cache}}
=
[\tilde k_1;\,\tilde k_2;\,\cdots;\,\tilde k_i]
$$

$$
V_{\text{cache}}=[v_1;\,v_2;\,\cdots;\,v_i]
$$

默认情况下，当前 token 的注意力会使用当前 query 和缓存中的所有 `Key/Value`：

$$
S_i
=
\frac{\tilde q_iK_{\text{cache}}^T}{\sqrt{d_k}}
$$

$$
A_i = softmax(S_i)
$$

$$
O_i = A_iV_{\text{cache}}
$$

因此推理时的单步注意力可以理解为：

$$
O_i
=
\operatorname{Attention}
(\tilde q_i,K_{\text{cache}},V_{\text{cache}})
$$

这等价于完整因果注意力矩阵中的第 $i$ 行。区别是推理时不会重复计算历史 token 的 $K$ 和 $V$，而是直接复用缓存。

如果模型使用 `Sliding Window Attention`，推理时通常只使用最近窗口内的 `KV Cache`：

$$
O_i
=
\operatorname{Attention}
\left(
\tilde q_i,
K_{\max(1,i-w+1):i},
V_{\max(1,i-w+1):i}
\right)
$$

其中 $w$ 表示 `window_size`。

### 和训练时的区别

- 训练时：一次性计算完整的 $\tilde Q\tilde K^T$，得到 $T\times T$ 注意力矩阵，并使用 causal mask 屏蔽未来位置。
- 推理时：每一步只计算当前 token 的一行注意力，历史 token 的 key 和 value 来自`KV Cache`。
- 如果模型使用 `window_size`，训练和推理都会遵守相同的窗口限制。
- `KV Cache`节省的是历史$\tilde K$和$V$的重复计算；如果没有窗口限制，当前 token 仍然需要和全部缓存位置做 attention。

### 从 Logits 生成下一个 Token

完成当前步计算后，通常只取最后一个位置的 logits：

$$
logits_{\text{last}}
\in
\mathbb{R}^{B\times V_{\text{vocab}}}
$$

然后使用`argmax`、`temperature`、`top-k`或`top-p`等策略选择下一个 token。新 token 会作为下一次`Decode`的输入，循环执行直到生成终止 token 或达到长度限制。

## 监督微调 Supervised Fine-Tuning

`SFT`使用高质量的指令与回答数据继续训练预训练模型，使模型学习指令遵循和目标回答格式。

### 训练数据

一条 SFT 数据通常包含：

```text
System: 你是一个有帮助的助手。
User: 请解释什么是自注意力。
Assistant: 自注意力是一种……
```

经过 chat template 和 tokenizer 处理后，整段对话被拼接成一个 token 序列：

$$
x=[x_1,x_2,\ldots,x_T]
$$

模型仍然执行 next-token prediction，SFT 并没有改变语言模型的基本自回归目标。

### Targets 和 Loss Mask

targets 仍由输入序列右移一位得到：

$$
targets_t=x_{t+1}
$$

但 SFT 通常只计算`Assistant`回答部分的 loss，`System`、`User`和 padding 位置不参与：

$$
m_t
=
\begin{cases}
1, & targets_t\text{属于 Assistant 回答}\\
0, & \text{otherwise}
\end{cases}
$$

最终损失为：

$$
Loss
=
\frac{\sum_t m_t\ell_t}
{\sum_t m_t}
$$

模型可以利用整段对话作为上下文，但梯度主要来自需要学习生成的回答位置。

### Teacher Forcing

训练时，每个位置接收的是数据中的真实历史 token，而不是模型上一步生成的 token，这称为`Teacher Forcing`。

因此，长度为$T$的样本可以并行计算所有有效位置的 next-token loss，不需要像推理一样逐 token 生成。

### 多轮对话

多轮对话可以只训练最后一轮回答，也可以训练所有`Assistant`回答：

```text
System    -> 不计算 loss
User      -> 不计算 loss
Assistant -> 计算 loss
User      -> 不计算 loss
Assistant -> 计算 loss
```

具体哪些位置参与 loss 由数据构造和 loss mask 策略决定。

### 和预训练的区别

- 预训练主要使用大规模语料学习语言规律和知识。
- SFT 使用规模更小、质量更高的指令回答数据学习指令遵循。
- 两者通常使用相同的自回归交叉熵目标，主要区别是训练数据及参与 loss 的 token 范围。

## Gated DeltaNet 线性注意力

`Gated DeltaNet`用固定大小的记忆矩阵压缩历史信息，不构造 $T\times T$注意力矩阵。其核心包含：

- `decay gate`：整体衰减旧状态。
- `delta rule`：定向修改与当前 key 相关的记忆。

部分实现还在线性注意力前加入因果短卷积，以混合局部上下文。以下以`Qwen3.5`的实现为例。

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

### 因果短卷积

在执行`delta rule`前，对拼接的`Q/K/V`投影应用逐通道因果一维卷积。设：

$$
u_t=W_{qkv}x_t,
\qquad
u_t\in\mathbb{R}^{D_{qkv}}
$$

卷积核长度为 $K$，各位置的逐通道权重为 $c_j\in\mathbb{R}^{D_{qkv}}$。卷积和拆分过程为：

$$
\bar u_t
=
\operatorname{SiLU}
\left(
\sum_{j=0}^{K-1}c_j\odot u_{t-j}
\right)
$$

$$
q_t,k_t,v_t
=
\operatorname{Split}(\bar u_t)
$$

其中，当 $t-j<0$ 时令 $u_{t-j}=0$；$\odot$表示逐元素乘法。输出只依赖区间 $[t-K+1,t]$，因此满足因果约束，并为`Q/K/V`提供长度为 $K$ 的局部感受野。

解码时只需缓存最近 $K-1$ 个 $u_t$，卷积状态大小为 $O((K-1)D_{qkv})$，不随上下文长度增长。卷积状态保存短期局部信息，矩阵状态 $M_t$压缩更长的历史信息。

### 复杂度

按单个 head 估算，并忽略线性投影的开销，长度为 $T$ 的序列所需计算量和状态空间分别为：

$$
O\left(Td_kd_v+TKD_{qkv}\right),
\qquad
O\left(d_kd_v+(K-1)D_{qkv}\right)
$$

当 $K$ 固定时，计算量随 $T$ 线性增长，状态空间与 $T$ 无关。训练时可并行计算卷积，并分块并行计算`delta rule`；解码时只更新卷积状态和矩阵状态，无需维护随上下文增长的`KV Cache`。

### 和 Softmax Attention 的区别

- `Softmax Attention` 能直接访问每个历史 token，但计算量和 `KV Cache` 随序列长度增长。
- `Gated DeltaNet` 使用固定大小的状态，长序列推理更节省显存，但可能发生记忆冲突。
- 两者可以组成混合模型：`Gated DeltaNet` 压缩长期信息，`Sliding Window Attention` 建模局部依赖。
