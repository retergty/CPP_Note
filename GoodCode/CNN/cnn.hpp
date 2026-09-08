#pragma once

/*
 * Conv1d + 激活 —— 边界剥离 + 寄存器分块 + Cache 分块
 *
 * 计算  out[out_ch][out_len] = act(conv1d(in[in_ch][in_len], w) + b)
 *
 * 存储约定：全部行主序、通道优先
 *   in [i, x]    = in[i * in_len + x]
 *   w  [o, i, k] = w[(o * in_ch + i) * kernel + k]
 *   b  [o]
 *   out[o, t]    = out[o * out_len + t]
 *   窗口起点 x0 = t * stride - pad，第 k 个 tap 落在 x0 + k
 *
 * pad_val 为逐输入通道的填充值（nullptr 表示零填充）。原项目里 conv1 用输入
 * 序列的均值填充、conv2/conv3 用零填充，这个参数就是为此保留的。
 *
 * ===========================================================================
 * 优化过程
 * ===========================================================================
 *
 * 0) 朴素四重循环
 *
 *      for o: for t: for i: for k:
 *        out[o][t] += w[o][i][k] * in[i][t*stride - pad + k]   // 每 tap 判越界
 *
 *    三个问题：越界分支在最内层，向量化无从谈起；out[o][t] 每个 tap 读改写一
 *    次内存；in 的同一段数据被 out_ch 个输出通道各自重新拉一遍。
 *
 * 1) 边界剥离（head / body / tail）
 *
 *    只有 t 贴近两端时窗口才会越界。让窗口整个落在 in 内：
 *      t*stride - pad >= 0
 *      t*stride - pad + kernel - 1 <= in_len - 1
 *    解得
 *      t ∈ [ ceil(pad/stride), floor((in_len - kernel + pad)/stride) ]
 *
 *    这段区间叫 body，用完全不判越界的快路径；区间之外才走带 pad_val 的慢路
 *    径。典型参数（kernel=9, stride=2, pad=4, in_len 上千）下 body 覆盖 99%
 *    以上的输出点，等于把最内层的分支整个删掉。
 *
 * 2) 寄存器分块 / 延迟隐藏：一次算 R 个输出点
 *
 *    这一步的主要动机是 FMA 延迟，不是访存。只用一个累加器时，内层是
 *      acc = fma(wk, x, acc)
 *    下一条 FMA 要等上一条的结果写回才能开始，整个 k 循环串成一条依赖链。
 *    VFMA 延迟 L 个周期，吞吐就被锁死在「每 L 周期一条」——哪怕流水线每周期
 *    都能发射一条，也有 L-1 个发射槽是空的。
 *
 *    改用 R 个互不相干的累加器，R 条链交替喂进同一条流水线，只要
 *      R >= L × 每周期可发射的 FMA 条数
 *    就能把发射槽填满。目标核 VFMA 延迟 4 周期、每周期 1 条，所以 R = 4
 *    （detail::kBodyUnroll）。换到有 2 条 FMA 流水的核上应取 8。
 *    R 的上限是寄存器数量：展开过头会 spill，反而更慢。
 *
 *    顺带的好处：wi[k] 只 load 一次广播给 R 路；stride < kernel 时相邻输出点
 *    的输入窗口重叠，xi 的 cache line 跟着复用；R 个累加器全程待在寄存器里，
 *    bias 当初值、激活在写回时做，out 于是从「每 tap 一次读改写」降到「每点
 *    一次 store」。
 *
 *    余数按 R/2, R/4, ... , 1 逐级收敛（detail::BodyLadder），body 长度任意
 *    都能覆盖。收尾这几档填不满流水线，但最多只剩 R-1 个点，不影响大局。
 *
 * 3) Cache 分块：t 块在外，o 在内
 *
 *    一个 t 块 [t0, t1) 用到的输入只有 in_ch * (t_tile*stride + kernel) 个元
 *    素。把 t 放外层、o 放内层，这一小片输入被 out_ch 个输出通道连着读完，全
 *    程留在 L1；代价是整张权重表 out_ch*in_ch*kernel 每个 t 块要重扫一遍。
 *    所以 t_tile 的取法是：让「输入片 + 权重表」一起装进 L1/L2。权重表大于
 *    L2 时应调大 t_tile（摊薄权重重扫），反之调小。
 *
 * 4) restrict 与融合
 *
 *    in / w / b / out 互不重叠，用 restrict 告诉编译器。否则每次写 out 之后
 *    编译器都必须假定 in、w 可能被改动而重新 load，内层循环既不能向量化也做
 *    不了软流水 —— 这一条往往比前面几步加起来还关键。
 *    bias 与激活都融进累加器，不额外走一趟内存。
 *
 * 用法：
 *   using cnn::conv1d;
 *   const auto s = cnn::Conv1dShape::make(ic, il, oc, k, stride, pad);
 *   conv1d(in, w, b, out, s);                 // 融合 ReLU，零填充
 *   conv1d(in, w, b, out, s, pad_val);        // 融合 ReLU，逐通道均值填充
 *   conv1d<cnn::Identity>(in, w, b, out, s);  // 不带激活
 */

#include <algorithm>

#ifndef CNN_RESTRICT
#if defined(_MSC_VER)
#define CNN_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define CNN_RESTRICT __restrict__
#else
#define CNN_RESTRICT
#endif
#endif

namespace cnn {

// ---------------------------------------------------------------------------
// 激活策略：无状态仿函数，内联后不留痕迹
// ---------------------------------------------------------------------------
struct Relu {
  template <typename T> T operator()(T v) const { return v > T{} ? v : T{}; }
};

struct Identity {
  template <typename T> T operator()(T v) const { return v; }
};

// ---------------------------------------------------------------------------
// 形状
// ---------------------------------------------------------------------------

// PyTorch 口径：floor((in_len + 2*pad - kernel) / stride) + 1，取不到输出记 0。
inline int conv1d_out_len(int in_len, int kernel, int stride, int pad) {
  if (kernel <= 0 || stride <= 0) {
    return 0;
  }
  const int span = in_len + 2 * pad - kernel;
  return span < 0 ? 0 : span / stride + 1;
}

struct Conv1dShape {
  int in_ch;
  int in_len;
  int out_ch;
  int kernel;
  int stride;
  int pad;
  int out_len; // 输出点数，同时是 out 的行间距

  static Conv1dShape make(int in_ch, int in_len, int out_ch, int kernel,
                          int stride = 1, int pad = 0) {
    Conv1dShape s;
    s.in_ch = in_ch;
    s.in_len = in_len;
    s.out_ch = out_ch;
    s.kernel = kernel;
    s.stride = stride;
    s.pad = pad;
    s.out_len = conv1d_out_len(in_len, kernel, stride, pad);
    return s;
  }

  bool valid() const {
    return in_ch > 0 && in_len > 0 && out_ch > 0 && kernel > 0 && stride > 0 &&
           pad >= 0 && out_len > 0;
  }
};

namespace detail {

// 把形参放进非推导语境：否则显式传 nullptr 时编译器会拿 std::nullptr_t 去推
// 导 T，与前面几个指针推出的 T 冲突而报 deduction failed。
template <typename T> struct NoDeduce {
  typedef T type;
};

// 向下取整除法（b > 0，a 可为负）。C++ 的 / 向零截断，被除数为负时会算大一格
// （如 -1/2 得 0，应为 -1），主体区间上界因此会多出一个并不存在的输出点，快路
// 径就会不判越界地读到 in 之外。这里必须用 floor 而不是 /。
inline int floor_div(int a, int b) {
  const int q = a / b;
  return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

inline int ceil_div(int a, int b) { return -floor_div(-a, b); }

// 半开区间 [begin, end)：窗口整个落在 in 内、无需任何越界判断的 t。
struct BodyRange {
  int begin;
  int end;
};

inline BodyRange body_range(const Conv1dShape &s) {
  int begin = std::max(ceil_div(s.pad, s.stride), 0);
  int end =
      std::min(floor_div(s.in_len - s.kernel + s.pad, s.stride) + 1, s.out_len);
  if (begin >= end) {
    return BodyRange{0, 0}; // 没有安全区，全部走慢路径
  }
  return BodyRange{begin, end};
}

// 慢路径：单个输出点，逐 tap 判越界，越界取 pad_val[i]（nullptr 则 0）。
template <typename T, typename Act>
T conv_point(const T *CNN_RESTRICT in, const T *CNN_RESTRICT wo, T bias,
             const Conv1dShape &s, int t, const T *pad_val, Act act) {
  T acc = bias;
  const int x0 = t * s.stride - s.pad;
  for (int i = 0; i < s.in_ch; ++i) {
    const T *xi = in + i * s.in_len;
    const T *wi = wo + i * s.kernel;
    for (int k = 0; k < s.kernel; ++k) {
      const int idx = x0 + k;
      // 转 unsigned 后一次比较同时判掉 idx < 0 和 idx >= in_len。
      const T x = static_cast<unsigned>(idx) < static_cast<unsigned>(s.in_len)
                      ? xi[idx]
                      : (pad_val != nullptr ? pad_val[i] : T{});
      acc += wi[k] * x;
    }
  }
  return act(acc);
}

// 主体展开度。取值须 >= VFMA 延迟 × 每周期可发射的 FMA 条数，否则累加器的
// 依赖链填不满流水线（见文件头 2）。目标核 VFMA 延迟 4 周期、单条 FMA 流水，
// 故为 4；双流水的核改 8。必须是 2 的幂，且受寄存器数量约束别开太大。
const int kBodyUnroll = 4;

// 快路径：一次算 R 个连续输出点，调用方保证 [t, t+R) 全在 body 内。
// 内层对 r 的循环长度是编译期常量，指望编译器把它完全展开、再把 acc[R] 拆成
// R 个标量提升进寄存器，从而得到 R 条互不依赖的累加链来隐藏 VFMA 延迟。
// 换编译器/目标时值得反汇编确认一下：若 r 循环没展开，acc[] 会留在栈上退化成
// load-FMA-store，这一步的收益就全没了。wk 一次 load 广播给 R 路。
template <int R, typename T, typename Act>
void conv_block(const T *CNN_RESTRICT in, const T *CNN_RESTRICT wo, T bias,
                const Conv1dShape &s, int t, T *CNN_RESTRICT yo, Act act) {
  T acc[R];
  for (int r = 0; r < R; ++r) {
    acc[r] = bias;
  }
  const int x0 = t * s.stride - s.pad;
  for (int i = 0; i < s.in_ch; ++i) {
    const T *xi = in + i * s.in_len + x0;
    const T *wi = wo + i * s.kernel;
    for (int k = 0; k < s.kernel; ++k) {
      const T wk = wi[k];
      for (int r = 0; r < R; ++r) {
        acc[r] += wk * xi[k + r * s.stride];
      }
    }
  }
  for (int r = 0; r < R; ++r) {
    yo[t + r] = act(acc[r]);
  }
}

// 展开梯度 R, R/2, ... , 1：先用满宽度吃掉主体，再逐级收掉余数。
// t 按引用推进，递归到 0 终止。
template <int R> struct BodyLadder {
  template <typename T, typename Act>
  static void run(const T *CNN_RESTRICT in, const T *CNN_RESTRICT wo, T bias,
                  const Conv1dShape &s, int &t, int body_end,
                  T *CNN_RESTRICT yo, Act act) {
    for (; t + R <= body_end; t += R) {
      conv_block<R>(in, wo, bias, s, t, yo, act);
    }
    BodyLadder<R / 2>::run(in, wo, bias, s, t, body_end, yo, act);
  }
};

template <> struct BodyLadder<0> {
  template <typename T, typename Act>
  static void run(const T *, const T *, T, const Conv1dShape &, int &, int, T *,
                  Act) {}
};

} // namespace detail

// ---------------------------------------------------------------------------
// Conv1d + 激活
//
//   in       [in_ch][in_len]
//   w        [out_ch][in_ch][kernel]
//   b        [out_ch]
//   out      [out_ch][s.out_len]
//   pad_val  [in_ch]，nullptr 表示零填充
//   t_tile   输出点的 cache 分块宽度，<= 0 表示不分块
//
// 模板参数 R 是主体的展开度，跟着目标核的 VFMA 延迟走，见 detail::kBodyUnroll。
// 前置条件：in / w / b / out 四段内存互不重叠（内部按 restrict 处理）。
// ---------------------------------------------------------------------------
template <typename Act = Relu, int R = detail::kBodyUnroll, typename T>
void conv1d(const T *CNN_RESTRICT in, const T *CNN_RESTRICT w,
            const T *CNN_RESTRICT b, T *CNN_RESTRICT out, const Conv1dShape &s,
            typename detail::NoDeduce<const T *>::type pad_val = nullptr,
            int t_tile = 32, Act act = Act{}) {
  if (!s.valid()) {
    return;
  }
  if (t_tile <= 0) {
    t_tile = s.out_len;
  }
  const detail::BodyRange body = detail::body_range(s);

  for (int t0 = 0; t0 < s.out_len; t0 += t_tile) {
    const int t1 = std::min(t0 + t_tile, s.out_len);
    const int head_end = std::min(t1, body.begin); // 块内头部的终点
    const int body_end = std::min(t1, body.end);   // 块内主体的终点

    for (int o = 0; o < s.out_ch; ++o) {
      const T *wo = w + o * s.in_ch * s.kernel;
      T *yo = out + o * s.out_len;
      const T bo = b[o];

      int t = t0;
      for (; t < head_end; ++t) { // 头部：窗口左端越界
        yo[t] = detail::conv_point(in, wo, bo, s, t, pad_val, act);
      }
      // 主体：R 路并行填 FMA 流水线，余数按 R/2, R/4, ... , 1 收敛
      detail::BodyLadder<R>::run(in, wo, bo, s, t, body_end, yo, act);
      for (; t < t1; ++t) { // 尾部：窗口右端越界
        yo[t] = detail::conv_point(in, wo, bo, s, t, pad_val, act);
      }
    }
  }
}

} // namespace cnn
