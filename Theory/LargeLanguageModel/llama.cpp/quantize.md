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

#### 反量化公式

$$
\hat x_{j,i} = d \cdot s_j \cdot q_{j,i} - d_{min} \cdot m_j
$$

其中

* $q_{j,i} \in \{0,...,15\}$: `4-bit`权重值.
* $s_j \in \{0,..,63\}$:`6-bit`局部`scale`.
* $m_j \in \{0,...,63\}$:`6-bit`局部`offset`.
* $d,d_{min}$:整个256元素共享的FP16缩放因子.

#### 量化过程

##### 源码

```CPP
void quantize_row_q4_K_ref(const float * GGML_RESTRICT x, block_q4_K * GGML_RESTRICT y, int64_t k) {
    assert(k % QK_K == 0);
    const int nb = k / QK_K;

    uint8_t L[QK_K];
    uint8_t Laux[32];
    float   weights[32];
    float mins[QK_K/32];
    float scales[QK_K/32];

    for (int i = 0; i < nb; i++) {
        float max_scale = 0; // as we are deducting the min, scales are always positive
        float max_min = 0;
        for (int j = 0; j < QK_K/32; ++j) {
            //scales[j] = make_qkx1_quants(32, 15, x + 32*j, L + 32*j, &mins[j], 9, 0.5f);
            float sum_x2 = 0;
            for (int l = 0; l < 32; ++l) sum_x2 += x[32*j + l] * x[32*j + l];
            float av_x = sqrtf(sum_x2/32);
            for (int l = 0; l < 32; ++l) weights[l] = av_x + fabsf(x[32*j + l]);
            scales[j] = make_qkx2_quants(32, 15, x + 32*j, weights, L + 32*j, &mins[j], Laux, -1.f, 0.1f, 20, false);
            float scale = scales[j];
            if (scale > max_scale) {
                max_scale = scale;
            }
            float min = mins[j];
            if (min > max_min) {
                max_min = min;
            }
        }

        float inv_scale = max_scale > 0 ? 63.f/max_scale : 0.f;
        float inv_min   = max_min   > 0 ? 63.f/max_min   : 0.f;
        for (int j = 0; j < QK_K/32; ++j) {
            uint8_t ls = nearest_int(inv_scale*scales[j]);
            uint8_t lm = nearest_int(inv_min*mins[j]);
            ls = MIN(63, ls);
            lm = MIN(63, lm);
            if (j < 4) {
                y[i].scales[j] = ls;
                y[i].scales[j+4] = lm;
            } else {
                y[i].scales[j+4] = (ls & 0xF) | ((lm & 0xF) << 4);
                y[i].scales[j-4] |= ((ls >> 4) << 6);
                y[i].scales[j-0] |= ((lm >> 4) << 6);
            }
        }
        y[i].d = GGML_FP32_TO_FP16(max_scale/63.f);
        y[i].dmin = GGML_FP32_TO_FP16(max_min/63.f);

        uint8_t sc, m;
        for (int j = 0; j < QK_K/32; ++j) {
            get_scale_min_k4(j, y[i].scales, &sc, &m);
            const float d = GGML_FP16_TO_FP32(y[i].d) * sc;
            if (!d) continue;
            const float dm = GGML_FP16_TO_FP32(y[i].dmin) * m;
            for (int ii = 0; ii < 32; ++ii) {
                int l = nearest_int((x[32*j + ii] + dm)/d);
                l = MAX(0, MIN(15, l));
                L[32*j + ii] = l;
            }
        }

        uint8_t * q = y[i].qs;
        for (int j = 0; j < QK_K; j += 64) {
            for (int l = 0; l < 32; ++l) q[l] = L[j + l] | (L[j + l + 32] << 4);
            q += 32;
        }

        x += QK_K;
    }
}
```

##### 过程详解

优化目标是

$$
\begin{aligned}
\min_{\,d,\,d_{\min},\,\{s_j,m_j\},\,\{q_{j,i}\}}
&\quad
\sum_{j=0}^{7}\sum_{i=0}^{31}
w_{j,i}\Bigl[
  x_{j,i} - \bigl(d\,s_j\,q_{j,i} - d_{\min}\,m_j\bigr)
\Bigr]^2 \\[0.5em]
\text{s.t.}
&\quad q_{j,i}\in\{0,1,\ldots,15\},\quad
       j\in\{0,\ldots,7\},\; i\in\{0,\ldots,31\} \\
&\quad s_j,\,m_j\in\{0,1,\ldots,63\},\quad
       j\in\{0,\ldots,7\} \\
&\quad d,\,d_{\min}\in\mathrm{FP16}.
\end{aligned}
$$

这是一个很复杂的优化问题，所以`llama`对这个进行了降维处理.

```CPP
float sum_x2 = 0;
for (int l = 0; l < 32; ++l) sum_x2 += x[32*j + l] * x[32*j + l];
float av_x = sqrtf(sum_x2/32);
for (int l = 0; l < 32; ++l) weights[l] = av_x + fabsf(x[32*j + l]);
scales[j] = make_qkx2_quants(32, 15, x + 32*j, weights, L + 32*j, &mins[j], Laux, -1.f, 0.1f, 20, false);
```

第一阶段,对每个子块求解优化问题

$$
\min_{a,b,q_i} \sum_i^{31}w_i(x_i-(aq_i+b))^2 \\
\text{s.t} \quad q_i\in\{0,1,\ldots,15\} \quad b \leq 0
$$

$b \leq 0$的约束是因为保证量化区间包含零，减少元数据并简化点积内核。

$w_i$是一个权重，与$\vert x_i \vert$有关。

最后得到8组临时局部参数:

$$
a_0 ,\ldots ,a_7 \\
\mu_0,\ldots,\mu_7 \quad \mu_j = -b_j
$$

```CPP
float inv_scale = max_scale > 0 ? 63.f/max_scale : 0.f;
float inv_min   = max_min   > 0 ? 63.f/max_min   : 0.f;

uint8_t ls = nearest_int(inv_scale*scales[j]);
uint8_t lm = nearest_int(inv_min*mins[j]);

y[i].d = GGML_FP32_TO_FP16(max_scale/63.f);
y[i].dmin = GGML_FP32_TO_FP16(max_min/63.f);
```

第二阶段，量化$a_j$和$\mu_j$为`6-bit`.

首先找出

$$
a_{max} = \max_j a_j \\
\mu_{max} = \max_j \mu_j
$$

计算超块中$d$和$d_{min}$.

$$
d = \frac{a_{max}}{63} \\
\quad \\
d_{min} = \frac{\mu_{max}}{63}
$$

局部参数量化为

$$
\begin{aligned}
s_j &= \operatorname{clip}\!\left(\operatorname{round}\!\left(\frac{a_j}{d}\right),\,0,\,63\right) \\
m_j &= \operatorname{clip}\!\left(\operatorname{round}\!\left(\frac{\mu_j}{d_{\min}}\right),\,0,\,63\right)
\end{aligned}
$$

```CPP
get_scale_min_k4(j, y[i].scales, &sc, &m);
const float d = GGML_FP16_TO_FP32(y[i].d) * sc;
const float dm = GGML_FP16_TO_FP32(y[i].dmin) * m;
for (int ii = 0; ii < 32; ++ii) {
    int l = nearest_int((x[32*j + ii] + dm)/d);
    l = MAX(0, MIN(15, l));
    L[32*j + ii] = l;
}
```

第三阶段，用最终参数重新量化权重.

之前得到的临时$q_i$是基于高精度$a_j,\mu_j$计算的，需要重新计算

$$
q_{j,i} = \operatorname{clip}\!\left(\operatorname{round}\!\left(\frac{x_{j,i}+d_{\min}\,m_j}{d\,s_j}\right),\,0,\,15\right)
$$

```CPP
uint8_t * q = y[i].qs;
for (int j = 0; j < QK_K; j += 64) {
    for (int l = 0; l < 32; ++l) q[l] = L[j + l] | (L[j + l + 32] << 4);
    q += 32;
}
```

最后进行数据打包即可.

#### 推理计算时

在实际推理的优化内核中，`Q4_K`权重通常不会先展开成完整的浮点数组，而是把激活按256个元素量化为`Q8_K`，直接执行整数点积，并在累加后乘回缩放因子。

设`Q4_K`权重超块和`Q8_K`激活超块分别为

$$
\hat w_{j,i}=d_w s_j q^w_{j,i}-d_{\min,w}m_j
$$

$$
\hat a_{j,i}=d_a q^a_{j,i}
$$

其中$q^w_{j,i}\in\{0,\ldots,15\}$是`Q4_K`权重码，$q^a_{j,i}\in\{-127,\ldots,127\}$是`Q8_K`激活码；$j\in\{0,\ldots,7\}$表示32元素子块，$i\in\{0,\ldots,31\}$。两者的点积可以展开为

$$
\begin{aligned}
\sum_{j=0}^{7}\sum_{i=0}^{31}\hat w_{j,i}\hat a_{j,i}
= {}&
d_a d_w
\sum_{j=0}^{7}s_j
\left(\sum_{i=0}^{31}q^w_{j,i}q^a_{j,i}\right) \\
&-d_a d_{\min,w}
\sum_{j=0}^{7}m_j
\left(\sum_{i=0}^{31}q^a_{j,i}\right).
\end{aligned}
$$

令

$$
P_j=\sum_{i=0}^{31}q^w_{j,i}q^a_{j,i},
\qquad
B_j=\sum_{i=0}^{31}q^a_{j,i},
$$

则一个超块的结果为

$$
\operatorname{dot}
=d_a d_w\sum_{j=0}^{7}s_jP_j
-d_a d_{\min,w}\sum_{j=0}^{7}m_jB_j.
$$

这里$P_j$是无符号`4-bit`权重码和有符号`8-bit`激活码的整数点积；$B_j$是激活码之和，用于计算`Q4_K`仿射偏移产生的修正项。

`Q8_K`已经保存了每16个激活码的和，因此不需要在点积内核中重新求和：

$$
B_j=\text{bsums}_{2j}+\text{bsums}_{2j+1}.
$$

不使用加速指令集可以写为

```CPP
void ggml_vec_dot_q4_K_q8_K_generic(int n, float * GGML_RESTRICT s, size_t bs, const void * GGML_RESTRICT vx, size_t bx, const void * GGML_RESTRICT vy, size_t by, int nrc) {
    assert(n % QK_K == 0);
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    const block_q4_K * GGML_RESTRICT x = vx;
    const block_q8_K * GGML_RESTRICT y = vy;

    const int nb = n / QK_K;

    static const uint32_t kmask1 = 0x3f3f3f3f;
    static const uint32_t kmask2 = 0x0f0f0f0f;
    static const uint32_t kmask3 = 0x03030303;

    uint32_t utmp[4];

    const uint8_t * scales = (const uint8_t*)&utmp[0];
    const uint8_t * mins   = (const uint8_t*)&utmp[2];

    int8_t  aux8[QK_K];
    int16_t aux16[8];
    float   sums [8];
    int32_t aux32[8];
    memset(sums, 0, 8*sizeof(float));

    float sumf = 0;
    for (int i = 0; i < nb; ++i) {
        const uint8_t * GGML_RESTRICT q4 = x[i].qs;
        const  int8_t * GGML_RESTRICT q8 = y[i].qs;
        memset(aux32, 0, 8*sizeof(int32_t));
        int8_t * GGML_RESTRICT a = aux8;
        for (int j = 0; j < QK_K/64; ++j) {
            for (int l = 0; l < 32; ++l) a[l] = (int8_t)(q4[l] & 0xF);
            a += 32;
            for (int l = 0; l < 32; ++l) a[l] = (int8_t)(q4[l]  >> 4);
            a += 32; q4 += 32;
        }
        memcpy(utmp, x[i].scales, 12);
        utmp[3] = ((utmp[2] >> 4) & kmask2) | (((utmp[1] >> 6) & kmask3) << 4);
        const uint32_t uaux = utmp[1] & kmask1;
        utmp[1] = (utmp[2] & kmask2) | (((utmp[0] >> 6) & kmask3) << 4);
        utmp[2] = uaux;
        utmp[0] &= kmask1;

        int sumi = 0;
        for (int j = 0; j < QK_K/16; ++j) sumi += y[i].bsums[j] * mins[j/2];
        a = aux8;
        int is = 0;
        for (int j = 0; j < QK_K/32; ++j) {
            int32_t scale = scales[is++];
            for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
            for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
            q8 += 8; a += 8;
            for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
            for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
            q8 += 8; a += 8;
            for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
            for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
            q8 += 8; a += 8;
            for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
            for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
            q8 += 8; a += 8;
        }
        const float d = GGML_CPU_FP16_TO_FP32(x[i].d) * y[i].d;
        for (int l = 0; l < 8; ++l) sums[l] += d * aux32[l];
        const float dmin = GGML_CPU_FP16_TO_FP32(x[i].dmin) * y[i].d;
        sumf -= dmin * sumi;
    }
    for (int l = 0; l < 8; ++l) sumf += sums[l];
    *s = sumf;
}
```

注意不同CPU/GPU后端的SIMD实现和数据重排方式不同，但计算的都是上面的等价公式。

##### 过程详解

```CPP
int8_t * GGML_RESTRICT a = aux8;
for (int j = 0; j < QK_K/64; ++j) {
    for (int l = 0; l < 32; ++l) a[l] = (int8_t)(q4[l] & 0xF);
    a += 32;
    for (int l = 0; l < 32; ++l) a[l] = (int8_t)(q4[l]  >> 4);
    a += 32; q4 += 32;
}
```

每个字节保存两个权重码：低4位是前32个元素，高4位是后32个元素。每轮将32字节解包为64个`int8_t`，最终得到按原始顺序排列的`aux8[256]`。

```CPP
memcpy(utmp, x[i].scales, 12);
utmp[3] = ((utmp[2] >> 4) & kmask2) | (((utmp[1] >> 6) & kmask3) << 4);
const uint32_t uaux = utmp[1] & kmask1;
utmp[1] = (utmp[2] & kmask2) | (((utmp[0] >> 6) & kmask3) << 4);
utmp[2] = uaux;
utmp[0] &= kmask1;
```

利用`0x3f`、`0x0f`和`0x03`掩码重新组合`scales[12]`中的`6-bit`参数。解包后`utmp`的字节布局为

$$
[s_0,\ldots,s_7,\;m_0,\ldots,m_7].
$$

```CPP
const uint8_t * scales = (const uint8_t *)&utmp[0];
const uint8_t * mins   = (const uint8_t *)&utmp[2];
```

```CPP
int sumi = 0;
for (int j = 0; j < QK_K/16; ++j) {
    sumi += y[i].bsums[j] * mins[j/2];
}
```

`bsums`按16个激活码求和，而$m_j$对应32个元素，因此相邻两个`bsums`共用一个$m_j$：

$$
\text{sumi}
=\sum_{j=0}^{7}m_j
\left(\text{bsums}_{2j}+\text{bsums}_{2j+1}\right)
=\sum_{j=0}^{7}m_jB_j.
$$

```CPP
a = aux8;
int is = 0;
for (int j = 0; j < QK_K/32; ++j) {
    int32_t scale = scales[is++];
    for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
    for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
    q8 += 8; a += 8;
    for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
    for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
    q8 += 8; a += 8;
    for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
    for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
    q8 += 8; a += 8;
    for (int l = 0; l < 8; ++l) aux16[l] = q8[l] * a[l];
    for (int l = 0; l < 8; ++l) aux32[l] += scale * aux16[l];
    q8 += 8; a += 8;
}
```

源码将每个32元素子块手动展开为4组相同的8元素计算。每组先计算$q^wq^a$，再乘以局部$s_j$并累加；8个累加通道之和即主项：

$$
\sum_{l=0}^{7}\text{aux32}_l
=\sum_{j=0}^{7}s_jP_j.
$$

```CPP
const float d = GGML_CPU_FP16_TO_FP32(x[i].d) * y[i].d;
for (int l = 0; l < 8; ++l) sums[l] += d * aux32[l];
const float dmin = GGML_CPU_FP16_TO_FP32(x[i].dmin) * y[i].d;
sumf -= dmin * sumi;
```

最后乘回两个量化块的缩放因子，并减去仿射偏移修正项。处理完所有超块后，将8个通道相加得到最终点积：

```CPP
for (int l = 0; l < 8; ++l) sumf += sums[l];
*s = sumf;
```

### Q8_K

`Q8_K`是一种`8-bit`的量化格式，主要给中间激活量化用，不是像`Q4_K`那样常存进`GGUF`的权重量化格式。

#### 超块结构

```CPP
typedef struct {
    float   d;              // delta
    int8_t  qs[256];       // quants
    int16_t bsums[16]; // sum of quants in groups of 16
} block_q8_K;
```

* `d`超块的`scale`.
* `qs`有符号的`8-bit`整数.
* `bsums`每`16`个`qs`的和.用于点积加速.

#### 反量化公式

$$
\hat x_j = d\cdot q_j
$$

```CPP
void dequantize_row_q8_K(const block_q8_K * GGML_RESTRICT x, float * GGML_RESTRICT y, int64_t k) {
    assert(k % QK_K == 0);
    const int64_t nb = k / QK_K;
    for (int i = 0; i < nb; i++) {
        for (int j = 0; j < QK_K; ++j) {
            *y++ = x[i].d * x[i].qs[j];
        }
    }
}
```

#### 量化过程

##### 源码

```CPP
void quantize_row_q8_K_ref(const float * GGML_RESTRICT x, block_q8_K * GGML_RESTRICT y, int64_t k) {
    assert(k % QK_K == 0);
    const int64_t nb = k / QK_K;

    for (int i = 0; i < nb; i++) {

        float max = 0;
        float amax = 0;
        for (int j = 0; j < QK_K; ++j) {
            float ax = fabsf(x[j]);
            if (ax > amax) {
                amax = ax; max = x[j];
            }
        }
        if (!amax) {
            y[i].d = 0;
            memset(y[i].qs, 0, QK_K);
            x += QK_K;
            continue;
        }
        //const float iscale = -128.f/max;
        // We need this change for IQ2_XXS, else the AVX implementation becomes very awkward
        const float iscale = -127.f/max;
        for (int j = 0; j < QK_K; ++j) {
            int v = nearest_int(iscale*x[j]);
            y[i].qs[j] = MIN(127, v);
        }
        for (int j = 0; j < QK_K/16; ++j) {
            int sum = 0;
            for (int ii = 0; ii < 16; ++ii) {
                sum += y[i].qs[j*16 + ii];
            }
            y[i].bsums[j] = sum;
        }
        y[i].d = 1/iscale;
        x += QK_K;
    }
}
```

##### 过程详解

```CPP
for (int j = 0; j < QK_K; ++j) {
    float ax = fabsf(x[j]);
    if (ax > amax) {
        amax = ax; max = x[j];
    }
    if (!amax) {
    y[i].d = 0;
    memset(y[i].qs, 0, QK_K);
    x += QK_K;
    continue;
    }
}
```

遍历超块，取绝对值最大的元素，记下其带符号值max，和绝对值amax.如果amax严格为零，清零qs跳过.

```CPP
const float iscale = -127.f/max;
for (int j = 0; j < QK_K; ++j) {
    int v = nearest_int(iscale*x[j]);
    y[i].qs[j] = MIN(127, v);
}
```

确定`iscale`和`d`。

$$
iscale = -\frac{127}{m} \\
d = \frac{1}{scale} = -\frac{m}{127}
$$

计算量化权重

$$
q_j = \min(127,\operatorname{round}(\operatorname{iscale}\cdot x))
$$

```CPP
for (int j = 0; j < QK_K/16; ++j) {
    int sum = 0;
    for (int ii = 0; ii < 16; ++ii) {
        sum += y[i].qs[j*16 + ii];
    }
    y[i].bsums[j] = sum;
}
y[i].d = 1/iscale;
```

填`bs`，是每16个qs的和.

写入`d`.
