#pragma once

/*
 * GEMM — k-i-j 顺序（以及切块）
 *
 * 计算  C[M×N] = A[M×K] * B[K×N]
 * 存储与 gemm_ikj.hpp 相同：行主序，A[i,k]=A[i*lda+k] 等。
 *
 * ---------------------------------------------------------------------------
 * 和 i-k-j 的差别
 *
 * 两边最内层都沿 j 走：C[i,:] += A[i,k] * B[k,:]，B 行、C 行连续。
 *
 * i-k-j：钉住一行 C，沿 k 累加。A 一行连续，C 一直热；B 每行 C 都重扫一遍。
 *
 * k-i-j：钉住一个 k，对所有行做 rank-1。
 *   B[k,:] 理论上只读一次，给每个 i 用；
 *   但不切块时，每个 k 都扫完整张 C，C 大于 cache 就会被冲掉，
 *   且 A[i,k] 是按列取（步长 lda）。
 *
 * 所以裸 kij 往往不如 ikj。切块之后 i 一次只走 tm 行，
 * B[k, j0:j_end] 复用还在，C 小块又装得下 —— 这才是 kij 快的前提，
 * 也是 ggml simd_gemm_ukernel 用 k-i-j 的原因（那里 tm 小到寄存器）。
 *
 * 用法：
 *   gemm::kij(M, N, K, A, B, C);
 *   gemm::kij(M, N, K, A, lda, B, ldb, C, ldc);
 *   gemm::kij_tiled(M, N, K, A, B, C);
 */

#include "gemm_common.hpp"

#include <algorithm>
#include <cstddef>

namespace gemm {
namespace detail {

// C[i0:i_end, j0:j_end] += A[i0:i_end, k0:k_end] * B[k0:k_end, j0:j_end]
// 块内 k-i-j：同一行 B 先 load，再广播给这一小段 i。
template <typename T>
void kij_tile(std::size_t i0, std::size_t i_end, std::size_t j0,
              std::size_t j_end, std::size_t k0, std::size_t k_end, const T *A,
              std::size_t lda, const T *B, std::size_t ldb, T *C,
              std::size_t ldc) {
  const std::size_t n_tile = j_end - j0;
  for (std::size_t k = k0; k < k_end; ++k) {
    const T *b_row = B + k * ldb + j0;
    for (std::size_t i = i0; i < i_end; ++i) {
      const T aik = A[i * lda + k];
      T *c_row = C + i * ldc + j0;
      for (std::size_t j = 0; j < n_tile; ++j) {
        c_row[j] += aik * b_row[j];
      }
    }
  }
}

} // namespace detail

// ---------------------------------------------------------------------------
// 不切块的 k-i-j（对照用：C 整表每个 k 扫一遍）
// ---------------------------------------------------------------------------
template <typename T>
void kij(std::size_t M, std::size_t N, std::size_t K, const T *A, std::size_t lda,
         const T *B, std::size_t ldb, T *C, std::size_t ldc) {
  detail::zero_matrix(M, N, C, ldc);
  detail::kij_tile(0, M, 0, N, 0, K, A, lda, B, ldb, C, ldc);
}

template <typename T>
void kij(std::size_t M, std::size_t N, std::size_t K, const T *A, const T *B,
         T *C) {
  kij(M, N, K, A, K, B, N, C, N);
}

// ---------------------------------------------------------------------------
// 切块 k-i-j
//
// 外层 k0-i0-j0：先钉住 B 的 k 面板，再让同一段 B 打在 C 的各个小块上。
// 块内仍是 k-i-j。tm 宜偏小（接近 ggml 的 RM），B 行复用才划算。
// ---------------------------------------------------------------------------
template <typename T>
void kij_tiled(std::size_t M, std::size_t N, std::size_t K, const T *A,
               std::size_t lda, const T *B, std::size_t ldb, T *C,
               std::size_t ldc, std::size_t tm = 8, std::size_t tn = 64,
               std::size_t tk = 64) {
  detail::zero_matrix(M, N, C, ldc);

  for (std::size_t k0 = 0; k0 < K; k0 += tk) {
    const std::size_t k_end = std::min(k0 + tk, K);
    for (std::size_t i0 = 0; i0 < M; i0 += tm) {
      const std::size_t i_end = std::min(i0 + tm, M);
      for (std::size_t j0 = 0; j0 < N; j0 += tn) {
        const std::size_t j_end = std::min(j0 + tn, N);
        detail::kij_tile(i0, i_end, j0, j_end, k0, k_end, A, lda, B, ldb, C,
                         ldc);
      }
    }
  }
}

template <typename T>
void kij_tiled(std::size_t M, std::size_t N, std::size_t K, const T *A,
               const T *B, T *C, std::size_t tm = 8, std::size_t tn = 64,
               std::size_t tk = 64) {
  kij_tiled(M, N, K, A, K, B, N, C, N, tm, tn, tk);
}

} // namespace gemm
