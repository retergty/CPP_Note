# LLM 视觉部分

## 动态分辨率预处理

动态分辨率预处理根据图像的原始尺寸和宽高比确定输入分辨率，使不同图像产生不同数量的视觉`Token`。其目标是在保留图像结构的同时控制计算量。

设原图尺寸为$H_0\times W_0$，预处理后的尺寸为$H\times W$，空间`Patch Size`为$P$，空间合并率为$M$。每个合并后的视觉`Token`在高、宽方向对应的像素跨度为：

$$
F=PM
$$

因此，视觉`Token`数量为：

$$
N_{\text{vision}}
=
\frac{H}{F}\times\frac{W}{F}
=
\frac{HW}{F^2}
$$

动态分辨率算法需要满足：

1. $H$和$W$能够被$F$整除；
2. $N_{\text{vision}}$位于给定的`Token`预算内；
3. $H/W$尽可能接近$H_0/W_0$。

设视觉`Token`数量限制为$N_{\min}$和$N_{\max}$，则允许的像素面积范围为：

$$
A_{\min}=N_{\min}F^2,
\qquad
A_{\max}=N_{\max}F^2
$$

首先将原图面积限制在该范围内：

$$
A_{\text{target}}
=
\operatorname{clip}(H_0W_0,A_{\min},A_{\max})
$$

然后计算等比例缩放系数：

$$
s=
\sqrt{\frac{A_{\text{target}}}{H_0W_0}}
$$

理想目标尺寸为$sH_0\times sW_0$。将其量化为$F$的整数倍：

$$
H=Q_F(sH_0),
\qquad
W=Q_F(sW_0)
$$

其中，$Q_F$表示按对齐单位$F$量化尺寸。量化后再次检查视觉`Token`数量；若超出预算，则在相邻的合法网格尺寸中选择宽高比误差最小的一组。最后将整张图像缩放到$H\times W$。

### 视觉 Token 数量

`Patch Embedding`产生的`Token`数量为：

$$
N_{\text{patch}}
=
\frac{H}{P}\times\frac{W}{P}
$$

每$M\times M$个相邻`Patch`合并为一个视觉`Token`，因此：

$$
N_{\text{vision}}
=
\frac{H}{PM}\times\frac{W}{PM}
=
\frac{HW}{(PM)^2}
=
\frac{HW}{F^2}
$$

`Qwen3.5`取$P=16$、$M=2$，因此$F=32$。默认像素预算对应约$64$至$16384$个视觉`Token`，实际输入长度随图像分辨率变化。

例如，$1080\times1920$的图像会对齐为$1088\times1920$，对应：

$$
N_{\text{vision}}
=
\frac{1088}{32}\times\frac{1920}{32}
=34\times60
=2040
$$

### 像素标准化

设缩放后的图像为$I$，其像素值位于$[0,255]$。将其转换为`RGB`后，使用：

$$
\mu=(0.5,0.5,0.5),
\qquad
\sigma=(0.5,0.5,0.5)
$$

将像素缩放到$[0,1]$并进行标准化：

$$
\hat I
=
\frac{I/255-\mu}{\sigma}
$$

对于单张图像，最终输出归一化后的图像张量及网格尺寸：

$$
\left(
1,\,
\frac{H}{P},\,
\frac{W}{P}
\right)
$$

其中第一个维度表示时间网格长度。网格尺寸用于后续`Patch Embedding`、空间合并和位置编码。

## Patch Embedding

`Transformer`接收向量序列，而归一化后的单张图像为三维像素张量：

$$
\hat I\in\mathbb{R}^{H\times W\times C}
$$

其中，$H$、$W$和$C$分别表示预处理后图像的高度、宽度和通道数。

`Patch Embedding`将图像划分为大小为$P\times P$的图像块，图像块数量为：

$$
N=\frac{H}{P}\times\frac{W}{P}
$$

每个图像块展平后表示为：

$$
x_i\in\mathbb{R}^{P^2C}
$$

再通过可学习的线性映射转换为$D$维视觉`Token`：

$$
e_i=x_iW_E+b_E,
\qquad
W_E\in\mathbb{R}^{P^2C\times D}
$$

最终得到视觉`Token`序列：

$$
E=[e_1,e_2,\ldots,e_N]\in\mathbb{R}^{N\times D}
$$

实际实现中通常使用卷积核大小和步长均为$P$的二维卷积，一次完成图像分块和线性映射。原始`ViT`使用互不重叠的图像块，其他视觉模型也可以通过减小步长生成相互重叠的图像块。

### 时间维 Patch Embedding

视频同时具有时间和空间结构，需要将相邻帧中相同空间位置的图像块联合编码。

设预处理后的视频包含$T$帧，输入张量为：

$$
V\in\mathbb{R}^{T\times H\times W\times C}
$$

设时间`Patch Size`为$P_t$，空间`Patch Size`为$P$。每个三维图像块的大小为：

$$
P_t\times P\times P
$$

展平后得到：

$$
x_i\in\mathbb{R}^{P_tP^2C}
$$

再通过可学习的线性映射转换为$D$维视觉`Token`：

$$
e_i=x_iW_E+b_E,
\qquad
W_E\in\mathbb{R}^{P_tP^2C\times D}
$$

三维图像块数量为：

$$
N
=
\frac{T}{P_t}
\times
\frac{H}{P}
\times
\frac{W}{P}
$$

最终得到：

$$
E=[e_1,e_2,\ldots,e_N]\in\mathbb{R}^{N\times D}
$$

输入帧数必须能被$P_t$整除；如果帧数不足，通常重复最后一帧补齐。对于单张图像，预处理会在时间维复制该图像。

## Vision Transformer

### 可学习绝对位置编码

`Patch Embedding`只包含图像块的内容，不包含其空间位置。视觉`Transformer`需要为每个`Patch`加入位置编码，以区分不同的行列坐标。

设预训练时使用的基础网格为$h_0\times w_0$，可学习位置编码为：

$$
E_0\in\mathbb{R}^{h_0\times w_0\times D}
$$

其中每个位置对应一个独立的$D$维可学习向量。对于网格大小固定的图像，可以将位置编码展平后直接加到视觉`Token`上：

$$
Z=E+\operatorname{Flatten}(E_0)
$$

动态分辨率会产生大小为$h\times w$的网格，其中：

$$
h=\frac{H}{P},
\qquad
w=\frac{W}{P}
$$

因此，需要在二维空间上对基础位置编码进行插值：

$$
E_{\text{pos}}
=
\operatorname{Interpolate}_{2D}(E_0,h,w)
\in
\mathbb{R}^{h\times w\times D}
$$

再将插值结果展平并加入视觉`Token`：

$$
Z=E+\operatorname{Flatten}(E_{\text{pos}})
$$

对于视频，时间网格长度为：

$$
t=\frac{T}{P_t}
$$

同一套空间位置编码沿时间维重复：

$$
Z
=
E+\operatorname{Repeat}
\left(
\operatorname{Flatten}(E_{\text{pos}}),t
\right)
$$

可学习绝对位置编码能够直接表示空间位置，但对未见过的分辨率依赖插值，且不直接表示两个位置之间的相对关系。因此，视觉`Transformer`通常还会在注意力的$Q$和$K$中引入二维旋转位置编码。

### M-RoPE

`M-RoPE`全称为`Multi-dimensional Rotary Position Embedding`，用于在语言模型中统一表示文本、图像和视频的位置。它作用于注意力的$Q$和$K$，与视觉编码器输入端的可学习绝对位置编码属于不同阶段。

图像展平为序列后，一维序号无法完整表示二维空间关系。例如，一行末尾和下一行开头在序列中相邻，但在图像中并不一定相邻。

`M-RoPE`为每个视觉`Token`分配三个位置坐标：

$$
(p_t,p_h,p_w)
$$

其中，$p_t$、$p_h$和$p_w$分别表示时间、高度和宽度位置。位置编号整体表示为：

$$
\operatorname{position\_ids}
\in
\mathbb{Z}^{3\times L}
$$

其中$L$是多模态输入序列长度。

对于文本`Token`，三个坐标使用相同的一维位置：

$$
(p_t,p_h,p_w)=(p,p,p)
$$

此时`M-RoPE`退化为普通的一维`RoPE`。

对于图像，设视觉`Token`在多模态序列中的起始位置为$p_0$，空间合并后的网格大小为：

$$
h=\frac{H}{PM},
\qquad
w=\frac{W}{PM}
$$

网格中第$(r,c)$个视觉`Token`的位置坐标为：

$$
(p_t,p_h,p_w)
=
(p_0,p_0+r,p_0+c)
$$

其中：

$$
0\leq r<h,
\qquad
0\leq c<w
$$

因此，同一行的视觉`Token`具有相同的高度坐标，同一列的视觉`Token`具有相同的宽度坐标。

例如，一个$2\times3$的视觉网格从$p_0=10$开始，其位置坐标依次为：

```text
(10, 10, 10)  (10, 10, 11)  (10, 10, 12)
(10, 11, 10)  (10, 11, 11)  (10, 11, 12)
```

后续文本从三个坐标中的最大位置继续编号，而不是简单增加视觉`Token`总数。这样可以在保留二维结构的同时避免把$h\times w$个视觉`Token`视为同样长度的一维位置跨度。

视频在此基础上增加时间位置。每个视觉`Token`通过$(p_t,p_h,p_w)$同时表示所属时间片和空间位置。`Qwen3.5`还使用显式时间戳`Token`标记视频时间，因此实际时间信息并不只依赖$p_t$。

#### Interleaved M-RoPE

早期`M-RoPE`将旋转维度连续划分为时间、高度和宽度三个区间：

```text
[T, T, T, ..., H, H, H, ..., W, W, W, ...]
```

由于`RoPE`的不同维度对应不同频率，这种连续划分会使三个坐标分别集中在不同频段，导致频率分布不均衡。

`Qwen3.5`采用`Interleaved M-RoPE`，将三个坐标交错分配到旋转维度：

```text
[T, H, W, T, H, W, T, H, W, ...]
```

设第$i$组旋转特征选择的坐标为$u_i$：

$$
u_i
\in
\{p_t,p_h,p_w\}
$$

则该组特征的旋转角度为：

$$
\phi_i=u_i\theta_i
$$

并分别对$Q$和$K$应用旋转：

$$
Q_i'=R(\phi_i)Q_i,
\qquad
K_i'=R(\phi_i)K_i
$$

交错分配使时间、高度和宽度信息都覆盖低频与高频维度，从而减少频谱偏置，改善高分辨率图像和长视频的位置建模。

整体流程可以概括为：

```text
文本 Token -> 一维位置 p -> (p, p, p)
图像 Token -> 二维网格位置 -> (p_t, p_h, p_w)
视频 Token -> 时间与空间位置 -> (p_t, p_h, p_w)
                    ↓
          Interleaved M-RoPE
                    ↓
          对 Attention 的 Q、K 旋转
```
