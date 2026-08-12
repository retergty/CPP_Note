# 量化quantize

## 常见概念

### 基本概念

用整数码$q$和少量参数近似浮点值$x$.

$$
\hat x = Dequant(q;\theta)
$$

$\theta$通常含尺度$s$,有时还有零点$z$.

### 量化误差

$$
e = x - \hat x
$$

### 对称量化

$$
x \approx s \cdot q
$$

其中$q$取对称整数集合$q \in \{-Q,...,Q \}$,零点在0.

尺度$s$常取

$$
s \approx \frac{\mathrm{max}_i \vert x \vert}{Q}
$$

量化:

$$
q = \mathrm{clamp}\left(\mathrm{round}\left(\frac{x}{s}\right), -Q, Q\right)
$$

反量化:

$$
\hat x = s \cdot q
$$

### 仿射量化

$$
x \approx s \cdot q + z
$$

其中$q$取非负整数集合$q \in \{0,...,Q\}$,偏置$z$使浮点零不必映射到整数零.

尺度$s$与偏置$z$常取

$$
s \approx \frac{\mathrm{max}_i x - \mathrm{min}_i x}{Q}, \quad z \approx \mathrm{min}_i x
$$

量化:

$$
q = \mathrm{clamp}\left(\mathrm{round}\left(\frac{x - z}{s}\right), 0, Q\right)
$$

反量化:

$$
\hat x = s \cdot q + z
$$

### 量化粒度

量化的粒度就是参数$\theta$(如$s$,$z$)共享的范围.设张量元素为$x_{c,i}$,其中$c$为通道下标,$i$为通道内元素下标.常见粒度:

* `per-tensor`: 整张量共用一组$\theta$

$$
\hat x_{c,i} = Dequant(q_{c,i};\theta)
$$

* `per-channel`: 每个通道$c$一组$\theta_c$

$$
\hat x_{c,i} = Dequant(q_{c,i};\theta_c)
$$

* `per-group`: 将每通道按组大小$G$分块,组$g=\lfloor i/G \rfloor$共用$\theta_{c,g}$

$$
\hat x_{c,i} = Dequant(q_{c,i};\theta_{c,\lfloor i/G \rfloor})
$$

粒度越细,量化误差通常越小,但额外参数与访存开销越大.

### 权重量化

只量化权重$W$,激活$A$仍用高精度(如 FP16/BF16)

$$
Y = Dequant(W_q)A
$$

### 权激活量化

权重$W$与激活$A$都量化,整数乘加后再按尺度还原.以对称量化为例:

$$
\hat W = s_W W_q, \quad \hat A = s_A A_q
$$

$$
Y = \hat W \hat A = s_W s_A\,(W_q A_q)
$$

其中$W_q A_q$在整数域完成乘加,$s_W s_A$在累加后一次性乘回.若用仿射量化,还需处理零点带来的纠偏项.

### PTQ（训后量化）

给定已训模型$f(x;W)$，求量化参数与码，使任务损失或重建误差可控.

## 常见量化

### Q4_K

`Q4_K`是一种`4-bit`的权重量化类型，特点是按行、按256超块存储，超块内再分8个32元子块，每个子块做仿射量化，再将子块中的scale/min再次量化为6-bit.有效约`4.5 bit`/权（含`scale`开销）

#### 超块结构

```CPP
struct block_q4_K {      // 144 bytes
    ggml_half d;         // 2B  超块 scale 步长
    ggml_half dmin;      // 2B  超块 min   步长
    uint8_t scales[12];  // 12B 8 组 (Ls, Lm)，各 6-bit
    uint8_t qs[128];     // 128B 256 个 4-bit q（两两一字节）
};
```

