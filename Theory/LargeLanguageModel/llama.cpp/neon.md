# ARM Neon

`NEON`是`ARM`的`SIMD`向量计算技术。

## 架构

### 寄存器

`AArch64`有`32`个`SIMD`/浮点寄存器

```text
V0, V1, V2, ... V31
```

每个寄存器宽度固定为`128 bit`,可以被解释为不同的数据格式，比如

例如同一个`V0`可以被看成：

* `V0.16B`: `16`个`8-bit`元素
* `V0.8H`: `8`个`16-bit`元素
* `V0.4S`: `4`个`32-bit`元素
* `V0.2D`: `2`个`64-bit`元素

### lane

每个寄存器里面的元素就叫做`lane`,是`SIMD`并行计算的基本单位

`SIMD`对每个`lane`独立计算,`lane`之间不会相互影响。

假设:

```text
a = [1, 2, 3, 4]
b = [10, 20, 30, 40]
```

执行

```CPP
float32x4_t c = vaddq_f32(a, b);
```

则是每个`lane`独立相加

```CPP
lane 0: 1 + 10 = 11
lane 1: 2 + 20 = 22
lane 2: 3 + 30 = 33
lane 3: 4 + 40 = 44

c = [11, 22, 33, 44]
```

### 过程调用约定

- `V0-V7`: 参数/返回, 调用者保存
- `V8-V15`: 被调用者保存 (只保证低 64-bit `Dn`)
- `V16-V31`: 调用者保存

## C语言封装

头文件`<arm_neon.h>`帮助进行Neon指令开发，不必手写汇编

### 向量类型命名

命名规则是`<base><count>_t`,例如`int8x16_t`,`float32x4_t`,`uint8x8_t`

| C 类型 | 元素 | 64-bit (D) | 128-bit (Q) |
|--------|------|------------|-------------|
| int8 | i8 | int8x8_t | int8x16_t |
| uint8 | u8 | uint8x8_t | uint8x16_t |
| int16 | i16 | int16x4_t | int16x8_t |
| uint16 | u16 | uint16x4_t | uint16x8_t |
| int32 | i32 | int32x2_t | int32x4_t |
| uint32 | u32 | uint32x2_t | uint32x4_t |
| int64 | i64 | int64x1_t | int64x2_t |
| uint64 | u64 | uint64x1_t | uint64x2_t |
| float16 | f16 | float16x4_t | float16x8_t |
| float32 | f32 | float32x2_t | float32x4_t |
| float64 | f64 | float64x1_t | float64x2_t |
| poly8/poly16 | 多项式 | 有 | 有 |
| bfloat16 | bf16 | bfloat16x4_t | bfloat16x8_t (需 FEAT_BF16) |

其中`Dn`是`Vn`的低`64`位，

### 结构体类型

```C
typedef struct int8x16x2_t { int8x16_t val[2]; } int8x16x2_t;
typedef struct int8x16x3_t { int8x16_t val[3]; } int8x16x3_t;
typedef struct int8x16x4_t { int8x16_t val[4]; } int8x16x4_t;
```

定义了一个结构体，单条`C`语句一次`load/store`多条向量

`int8x16x4_t`就是**4 个**独立的`int8x16_t`=**4 个 V 寄存器**,`.val[0]`..`.val[3]`各是一条`128-bit`向量.

### 内建函数

#### 命名规则

```text
v  +  <op>  +  [q]  +  [_n|_lane|_high|_low|_p64...]  +  _<type>
```

* `v`,`vector`向量
* `op`操作
* `q`操作整个`128-bit Vn`. 没有 `q` 则只操作低`64-bit Vn`.(Dn)
* `_n`一个标量广播到所有`lane`, 或乘标量.
* `_lane`取另一个向量的某个`lane`.
* `_low/_high`只操作`128-bit`的低/高`64-bit`.
* `_<type>`lane的元素类型，比如`_s8/_u8/_s16/_s32/_f32/_f16`

例如

```C
float32x4_t  a = vld1q_f32(p);           // load 4xf32
float32x4_t  b = vdupq_n_f32(3.f);       // broadcast
float32x4_t  c = vfmaq_f32(acc, a, b);   // acc + a*b  (fused)
float        s = vaddvq_f32(c);          // 水平求和 (ADDV)
int32x4_t    d = vdotq_s32(d, x8, y8);   // 4 组 int8 点积
```

#### 溢出回绕

通常内建函数的数学计算如果发生了溢出，会进行回绕，也就是同宽二补码取负，溢出回绕。

在`op`前面加入`q`字符的内建函数，执行饱和运算，溢出钳位到最大或者最小值.

例如

```CPP
int8x16_t t = vqaddq_s8(a, b);   // 127+1 -> 127，不是 -128
int8x16_t u = vqsubq_s8(a, b);
```

#### 元素变宽变窄

两个s8数相乘，如果想要不溢出的话，需要s16存储，也就是元素变宽.

在`op`后面加入`l`字符的算术内建函数，执行变宽操作，`lane`变宽

```CPP
int16x8_t c = vaddl_s8(a8, b8);
// a8、b8 是 int8x8_t（64-bit，8 个 int8）
// c    是 int16x8_t（128-bit，8 个 int16）
```

在`op`后面加入`w`字符的算术内建函数，执行第二个操作数变宽，第一个操作数已经是宽的计算

```CPP
c = vaddw_s8(acc16, b8); // acc 已是 int16，再加上扩展后的 b8
```

在`op`后面加入`n`字符的算术内建函数，执行窄化.lane变窄

```CPP
int16x8_t t = {100, 200, 300, -200, -300, 127, 128, -129};
c = vmovn_s16(t);                      // {100, -56, 44, 56, -44, 127, -128, 127}
c = vqmovn_s16(t);                     // {100, 127, 127, -128, -128, 127, 127, -12
```

#### 常见函数

##### 加载/存储

*  `vld1q_*`/`vst1q_*`加载或者存储整个`Vn`.

    ```CPP
    float32x4_t a = vld1q_f32(p);   // 读 p[0..3]，进 1 个 V
    vst1q_f32(p, a);                // 写回去

    int8x16_t  q = vld1q_s8(qs);    // 读 16 个 int8
    uint8x16_t u = vld1q_u8(qs);    // 同一 16 字节，当 uint8 看
    ```

* `vld1_*` / `vst1_*`加载或者存储`Dn`.

* `vld1q_*_x2/_x3/_x4`连续加载多个寄存器.

    ```CPP
    int8x16x2_t a = vld1q_s8_x2(p);   // 32 字节 -> 2 个 V
    int8x16x4_t b = vld1q_s8_x4(p);   // 64 字节 -> 4 个 V
    ```

* `vld1q_dup_*`从内存读一个元素，复制到该向量所有`lane`

    ```CPP
    float32x4_t s = vld1q_dup_f32(&scale);  // [s,s,s,s]
    ```

* `vld1q_lane_*`读一个元素，写进已有向量的某一个`lane`

    ```CPP
    float32x4_t v = ...;
    v = vld1q_lane_f32(p, v, 2);   // 只改 lane 2
    ```

* `vst1q_lane`只写回一个`lane`

    ```CPP
    vst1q_lane_s32(dst, vec, 0);   // 只把 vec 的 lane 0 写成一个 int32
    ```

##### 构造/拆解/搬移

* `vdupq_n_*/vmovq_n_*`将一个标量复制到lane

    ```CPP
    float32x4_t a = vdupq_n_f32(3.f);   // [3, 3, 3, 3]
    int8x16_t   m = vdupq_n_s8(8);      // 16 个 8，Q4 减 zero-point 用
    int32x4_t   z = vdupq_n_s32(0);     // 点积累加器清零
    ```

* `vgetq_lane_*`/`vsetq_lane_*`取出/修改一个lane

    ```CPP
    float s = vgetq_lane_f32(v, 2);          // 取出第 2 个 float
    int32x4_t v2 = vsetq_lane_s32(42, v, 1); // 只改 lane 1
    ```

* `vget_low_*`/`vget_high_*`获取低半高半部分的Vn.

    ```CPP
    int8x8_t lo = vget_low_s8(v);   // 字节 0..7   -> Dn 视图
    int8x8_t hi = vget_high_s8(v);  // 字节 8..15
    ```

* `vcombine_*`拼接为一个Vn

    ```CPP
    int8x16_t v2 = vcombine_s8(lo, hi);  // [lo | hi] 再拼成 128
    ```

*  `vextq_*`把`a`和`b`首尾相接，从偏移`n`起取一个向量宽度。`n`的单位是 该类型的`lane`数。

    ```CPP
    // a = [a0 a1 a2 a3]  b = [b0 b1 b2 b3]   都是 f32
    vextq_f32(a, b, 2);   // [a2, a3, b0, b1]
    vextq_f32(a, a, 2);   // [a2, a3, a0, a1]  旋转 2 个 lane
    ```

* `vzip1q/vzip2q`把`A`,`B`按`lane`穿插。`zip1`取交错后的前半，`zip2`取后半。

    ```CPP
    float32x4_t a = {0, 1, 2, 3};   // 示意，实际常用 vld1q
    float32x4_t b = {4, 5, 6, 7};
    float32x4_t z1 = vzip1q_f32(a, b);  // [0, 4, 1, 5]
    float32x4_t z2 = vzip2q_f32(a, b);  // [2, 6, 3, 7]
    ```

* `vuzp1q`/`vuzp2q`解穿插,偶位置进`uzp1`，奇位置进`uzp2`

    ```CPP
    float32x4_t ev = vuzp1q_f32(z1, z2);  // [0, 1, 2, 3]  回到 a
    float32x4_t od = vuzp2q_f32(z1, z2);  // [4, 5, 6, 7]  回到 b
    ```

* `vtrn1q`/`vtrn2q`成对转置，两个lane为一对

    ```CPP
    float32x4_t a = {0, 1, 2, 3};
    float32x4_t b = {4, 5, 6, 7};

    float32x4_t t1 = vtrn1q_f32(a, b);  // [0, 4, 2, 6]
    float32x4_t t2 = vtrn2q_f32(a, b);  // [1, 5, 3, 7]
    ```

* `vrev16`/`vrev32`/`vrev64`组内字节倒序

    ```CPP
    int8x16_t v = vld1q_s8(p);  // 字节 0,1,2,...,15

    int8x16_t r16 = vrev16q_s8(v);  // 1,0, 3,2, 5,4, ...
    int8x16_t r32 = vrev32q_s8(v);  // 3,2,1,0, 7,6,5,4, ...
    int8x16_t r64 = vrev64q_s8(v);  // 7,6,5,4,3,2,1,0, 再后 8 字节同样倒
    ```

* `vbslq`,bitwise select: `bsl(mask, a, b) = (mask & a) | (~mask & b)`

##### 整数浮点算数

* `vaddq`/`vsubq`逐`lane`加减.

    ```CPP
    int8x16_t  s = vaddq_s8(a, b);    // 16 个独立的 a[i]+b[i]，回绕
    int8x16_t  d = vsubq_s8(a, vdupq_n_s8(8));  // 16 个独立的 a[i] - 8,回绕
    float32x4_t f = vaddq_f32(x, y); // 4 个独立的 a[i] + b[i]

    int8x16_t t = vqaddq_s8(a, b);   // 127+1 -> 127，不是 -128
    int8x16_t u = vqsubq_s8(a, b);
    ```

* `vmulq`逐lane相乘

    ```CPP
    float32x4_t p = vmulq_f32(x, y);     // 4 个独立乘法
    int32x4_t   i = vmulq_s32(a, b);     // 两个 32 位整数相乘，数学上可以到 64 位，这里只取低 32 位，高位丢掉
    ```

* `vmlaq`/`vmlsq`乘加，比融合乘加`FMA`要慢.

    ```CPP
    // vmlaq_f32(acc, b, c) = acc + b*c
    acc = vmlaq_f32(acc, x, y);

    // vmlsq_f32(acc, b, c) = acc - b*c
    acc = vmlsq_f32(acc, x, y);

    // 每个 lane:  acc[i] + v[i] * s
    sumv = vmlaq_n_f32(sumv, vcvtq_f32_s32(dot), scale);
    ```

* `vdivq`逐lane相除

    ```CPP
    float32x4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t b = {10.f, 20.f, 30.f, 40.f};
    c = vdivq_f32(b, a);                   // {10, 10, 10, 10}
    ```

* `vsqrtq`逐lane开方

    ```CPP
    c = vsqrtq_f32((float32x4_t){1,4,9,16}); // {1, 2, 3, 4}
    ```

* `vabsq`逐lane取绝对值，溢出回绕

    ```CPP
    c = vabsq_s32((int32x4_t){-3, 4, INT32_MIN, 0});  // {3, 4, INT32_MIN, 0}
    c = vabsq_f32((float32x4_t){-1.5f, 2.f, -0.f, 3.f}); // {1.5, 2, 0, 3}
    c = vqabsq_s32((int32x4_t){-3, 4, INT32_MIN, 0}); // {3, 4, INT32_MAX, 0}
    ```

* `vnegq`逐lane取负，溢出回绕

    ```CPP
    c = vnegq_s32((int32x4_t){-3, 4, INT32_MIN, 0});  // {3, -4, INT32_MIN, 0}
    c = vnegq_f32((float32x4_t){-1.5f, 2.f, 0.f, 3.f});  // {1.5, -2, 0, -3}
    c = vqnegq_s32((int32x4_t){-3, 4, INT32_MIN, 0}); // {3, -4, INT32_MAX, 0}   -INT32_MIN 饱和成 +INT32_MAX
    ```

* `vabdq`逐lane进行abs(a-b)

    ```CPP
    c = vabdq_s16((int16x8_t){10,-4,7,0,1,1,1,1},
                (int16x8_t){ 3, 9,7,8,0,0,0,0});  // {7, 13, 0, 8, 1,1,1,1}
    c = vabdq_f32((float32x4_t){-1.5f, 2.f, 0.f, 3.f},
                (float32x4_t){ 0.5f, 5.f, 1.f, 1.f}); // {2, 3, 1, 2}
    ```

* `vmaxq`/`vmaxq`最大最小值.`vminnmq`会让数优先于NAN.

    ```CPP
    int16x8_t  a16 = {2, 3, 4, 5, -2, 100, 200, -5};
    int16x8_t  b16 = {3, 4, 5, 6, -3, 100, 200, -6};
    float32x4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t b = {10.f, 20.f, 30.f, 40.f};
    c = vmaxq_s16(a16, b16);               // {3, 4, 5, 6, -2, 100, 200, -5}
    c = vminq_s16(a16, b16);               // {2, 3, 4, 5, -3, 100, 200, -6}
    c = vmaxq_u16(vreinterpretq_u16_s16(a16), vreinterpretq_u16_s16(b16));
                                        // 有符号 -2 在 uint16 里是 65534
    c = vmaxq_f32(a, b);                   // {10, 20, 30, 40}
    c = vminq_f32((float32x4_t){1.f, NAN, 3.f, -0.f},
                (float32x4_t){2.f, 5.f, NAN,  0.f});   // lane1/2 为 NaN
    c = vminnmq_f32((float32x4_t){1.f, NAN, 3.f, -0.f},
                    (float32x4_t){2.f, 5.f, NAN,  0.f});  // {1, 5, 3, 0} 选数不选 NaN
    ```

* `vcvtq`/`vcvtnq`/`vcvtaq`/`vcvtmq`/`vcvtpq`进行整数浮点的转化，舍入方向不同.

    ```CPP
    c = vcvtq_f32_s32((int32x4_t){1, -2, 100, 0});     // {1, -2, 100, 0}

    float32x4_t x = {1.6f, 2.5f, -1.6f, -2.5f};
    c = vcvtq_s32_f32(x);                  // {1, 2, -1, -2}   向 0
    c = vcvtnq_s32_f32(x);                 // {2, 2, -2, -2}   nearest even；2.5 -> 2
    c = vcvtaq_s32_f32(x);                 // {2, 3, -2, -3}   ties away
    c = vcvtmq_s32_f32(x);                 // {1, 2, -2, -3}   toward -inf
    c = vcvtpq_s32_f32(x);                 // {2, 3, -1, -2}   toward +inf

    c = vcvtq_n_f32_s32((int32x4_t){256, 128, -256, 64}, 8);  // {1.0, 0.5, -1.0, 0.25}
    c = vcvtq_n_s32_f32((float32x4_t){1.5f, -1.5f, 0.25f, 2.f}, 8); // {384, -384, 64, 512}

    c = vcvt_f32_f16(vcvt_f16_f32(x));     // fp32 <-> fp16 往返
    ```

* `vfmaq_`/`vfmsq_`融合乘加，需要特殊的Feature.

    ```CPP
    float32x4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t b = {10.f, 20.f, 30.f, 40.f};

    float32x4_t acc = vdupq_n_f32(1.f);
    c = vfmaq_f32(acc, a, b);              // {11, 41, 91, 161}
    c = vfmsq_f32(acc, a, b);              // {-9, -39, -89, -159}
    c = vfmaq_n_f32(acc, a, 10.f);         // {11, 21, 31, 41}
    c = vfmaq_laneq_f32(acc, a, s, 2);     // {31, 61, 91, 121}  乘 s[2]=30
    ```

* `vpaddq`/`vpaddlq`相邻成对加

    ```CPP
    float32x4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t b = {10.f, 20.f, 30.f, 40.f};

    c = vpaddq_f32(a, b);                  // {1+2, 3+4, 10+20, 30+40} = {3, 7, 30, 60}
    c = vpaddlq_s16((int16x8_t){1,2,3,4,5,6,7,8}); // {3, 7, 11, 15}  加宽成 int32
    ```

* `vshrq_n_`右移

    ```CPP
    int16x8_t a = {256, 257, 511, 512, -16, -256, 1000, 8};

    c = vshrq_n_s16(a, 8);    // 仍是 int16x8_t
                          // {1, 1, 1, 2, -1, -1, 3, 0}
                          // 有符号：算术右移，符号位填入

    c = vshrn_n_s16(a, 8);    // 变成 int8x8_t
                          // {1, 1, 1, 2, -1, -1, 3, 0}
    ```

* `vaddvq`/`vaddlvq`/`vmaxvq`规约运算

    ```CPP
    float32x4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t b = {10.f, 20.f, 30.f, 40.f};
    f = vaddvq_f32(a);                     // 10
    f = vmaxvq_f32(a);                     // 4
    i = vaddlvq_s16((int16x8_t){1,2,3,4,5,6,7,8}); // 36，结果加宽成 int32
    ```
* `vmovn`宽 -> 窄

    ```CPP
    int16x8_t t = {100, 200, 300, -200, -300, 127, 128, -129};
    c = vmovn_s16(t);                      // {100, -56, 44, 56, -44, 127, -128, 127}
    c = vqmovn_s16(t);                     // {100, 127, 127, -128, -128, 127, 127, -128}
    ```

* `vdotq_`每`4`个`int8`点积进`1`个`int32`lane,需要`+dotprod`支持

    ```CPP
    int8x16_t u = {1,2,3,4, 5,6,7,8, 1,1,1,1, 2,2,2,2};
    int8x16_t v = {1,1,1,1, 1,1,1,1, 3,3,3,3, 4,4,4,4};
    c = vdotq_s32(vdupq_n_s32(0), u, v);   // {10, 26, 12, 32}

    /* 无 DotProd 时用 vmull_* + vpaddlq_* */
    c = vaddq_s32(vpaddlq_s16(vmull_s8(vget_low_s8(u),  vget_low_s8(v))),
                vpaddlq_s16(vmull_s8(vget_high_s8(u), vget_high_s8(v))));
                                        // 同样 {10, 26, 12, 32}
    ```

##### 半精度浮点FP16

半精度浮点FP16指的是16-bit的浮点数.

它的转换函数是

```CPP
float16x4_t h4 = vld1_f16(ptr);          // load 4 个 fp16
float32x4_t f  = vcvt_f32_f16(h4);       // 4 个 fp16 -> 4 个 fp32
float16x4_t b  = vcvt_f16_f32(f);        // 再转回去

float16x8_t h8 = vld1q_f16(ptr);
f = vcvt_f32_f16(vget_low_f16(h8));      // 低 4 个
f = vcvt_high_f32_f16(h8);               // 高 4 个
```

它的计算函数类似于上面的整数浮点算术，只是需要fp16的支持

```CPP
float16x8_t a = vld1q_f16(p);
float16x8_t b = vld1q_f16(q);

c = vaddq_f16(a, b);
c = vsubq_f16(a, b);
c = vmulq_f16(a, b);
c = vmulq_n_f16(a, 0.5f);          // 标量会按 fp16 语义用
c = vfmaq_f16(acc, a, b);          // acc + a*b，融合
c = vfmsq_f16(acc, a, b);
c = vdivq_f16(a, b);
c = vsqrtq_f16(a);
c = vabsq_f16(a);
c = vnegq_f16(a);
c = vabdq_f16(a, b);
c = vmaxq_f16(a, b);
c = vminq_f16(a, b);
c = vminnmq_f16(a, b);
```

##### 逻辑计算

##### 比较计算

##### 