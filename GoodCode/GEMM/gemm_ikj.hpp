#pragma once

/*
 * GEMM — i-k-j 顺序（以及切块）
 *
 * 计算  C[M×N] = A[M×K] * B[K×N]
 *
 * 存储约定：行主序（row-major）
 *   A[i, k] = A[i * lda + k]，lda >= K
 *   B[k, j] = B[k * ldb + j]，ldb >= N
 *   C[i, j] = C[i * ldc + j]，ldc >= N
 *
 * ---------------------------------------------------------------------------
 * 访存连续性（本文件的核心）
 *
 * 教科书三重循环是 i-j-k：
 *   for i:
 *     for j:
 *       for k:
 *         C[i, j] += A[i, k] * B[k, j]
 *
 * 最内层沿 k 走：A 的一行连续，但 B 每次跨 ldb 取同一列，
 * 步长是整行宽度，缓存行几乎用不上，B 变成随机访存。
 *
 * 改成 i-k-j（对 C 的一行做 rank-1 更新）：
 *   C[i, :] += A[i, k] * B[k, :]
 *
 * 最内层沿 j 走：B 的第 k 行、C 的第 i 行都是地址连续的。
 * A[i, k] 提出循环外，变成标量广播。
 *
 * 分块版本在块内仍用同一套 i-k-j，只是把工作集压进 cache，
 * 让同一段 A / B 被反复命中。连续性来自循环顺序，不是来自分块本身。
 *
 * 用法：
 *   gemm::ikj(M, N, K, A, B, C);                 // lda=K, ldb=N, ldc=N
 *   gemm::ikj(M, N, K, A, lda, B, ldb, C, ldc);  // 子矩阵 / 带 pitch
 *   gemm::ikj_tiled(M, N, K, A, B, C);           // 分块，大矩阵更友好
 */

#include "gemm_common.hpp"

#include <algorithm>
#include <cstddef>

namespace gemm {
namespace detail {

// C[i0:i_end, j0:j_end] += A[i0:i_end, k0:k_end] * B[k0:k_end, j0:j_end]
// 块内 i-k-j：沿 j 扫 B 行与 C 行，地址连续。
template <typename T>
void ikj_tile(std::size_t i0, std::size_t i_end, std::size_t j0,
              std::size_t j_end, std::size_t k0, std::size_t k_end, const T *A,
              std::size_t lda, const T *B, std::size_t ldb, T *C,
              std::size_t ldc) {
  const std::size_t n_tile = j_end - j0;
  for (std::size_t i = i0; i < i_end; ++i) {
    T *c_row = C + i * ldc + j0;
    const T *a_row = A + i * lda + k0;
    for (std::size_t kk = 0; kk < k_end - k0; ++kk) {
      const T aik = a_row[kk];
      const T *b_row = B + (k0 + kk) * ldb + j0;
      for (std::size_t j = 0; j < n_tile; ++j) {
        c_row[j] += aik * b_row[j];
      }
    }
  }
}

} // namespace detail

// ---------------------------------------------------------------------------
// 连续访存版：i-k-j
// ---------------------------------------------------------------------------
template <typename T>
void ikj(std::size_t M, std::size_t N, std::size_t K, const T *A, std::size_t lda,
         const T *B, std::size_t ldb, T *C, std::size_t ldc) {
  detail::zero_matrix(M, N, C, ldc);
  detail::ikj_tile(0, M, 0, N, 0, K, A, lda, B, ldb, C, ldc);
}

template <typename T>
void ikj(std::size_t M, std::size_t N, std::size_t K, const T *A, const T *B,
         T *C) {
  ikj(M, N, K, A, K, B, N, C, N);
}

// ---------------------------------------------------------------------------
// 分块版：外层切块进 cache，块内仍是连续的 i-k-j
//
// 外层 i0-j0-k0：C 的小块留在 cache 里，沿 k 累加。
// 默认 64：float 下一块约 16 KiB，三块量级仍常落在 L1/L2。
// ---------------------------------------------------------------------------
template <typename T>
void ikj_tiled(std::size_t M, std::size_t N, std::size_t K, const T *A,
               std::size_t lda, const T *B, std::size_t ldb, T *C,
               std::size_t ldc, std::size_t tm = 64, std::size_t tn = 64,
               std::size_t tk = 64) {
  detail::zero_matrix(M, N, C, ldc);

  for (std::size_t i0 = 0; i0 < M; i0 += tm) {
    const std::size_t i_end = std::min(i0 + tm, M);
    for (std::size_t j0 = 0; j0 < N; j0 += tn) {
      const std::size_t j_end = std::min(j0 + tn, N);
      for (std::size_t k0 = 0; k0 < K; k0 += tk) {
        const std::size_t k_end = std::min(k0 + tk, K);
        detail::ikj_tile(i0, i_end, j0, j_end, k0, k_end, A, lda, B, ldb, C,
                         ldc);
      }
    }
  }
}

template <typename T>
void ikj_tiled(std::size_t M, std::size_t N, std::size_t K, const T *A,
               const T *B, T *C, std::size_t tm = 64, std::size_t tn = 64,
               std::size_t tk = 64) {
  ikj_tiled(M, N, K, A, K, B, N, C, N, tm, tn, tk);
}

} // namespace gemm
