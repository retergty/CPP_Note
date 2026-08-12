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

在实际推理时`Q4_K`