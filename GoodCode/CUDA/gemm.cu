/*
 * CUDA GEMM：共享内存分块（Tiled Matrix Multiplication）
 *
 * 计算：
 *   C[M, N] = A[M, K] * B[K, N]
 *
 * 存储：
 *   矩阵均为 row-major。
 *   A[row, k] = A[row * K + k]
 *   B[k, col] = B[k * N + col]
 *   C[row, col] = C[row * N + col]
 *
 * 核心思路：
 *   1. 每个 block 计算 C 的一个 TILE × TILE 子块。
 *   2. A、B 按 K 维切块并加载到共享内存，减少全局内存访问。
 *   3. 一个线程计算 C 中的一个元素。
 *   4. 矩阵尺寸不是 TILE 整数倍时，越界位置补 0。
 *
 * 线程映射：
 *   blockIdx.y / threadIdx.y -> C 的行
 *   blockIdx.x / threadIdx.x -> C 的列
 */

#include <cuda_runtime.h>

constexpr int TILE = 16;

// 行主序矩阵乘法：C[M, N] = A[M, K] * B[K, N]。
// 一个thread负责一个C[row, col]的计算
// 使用a_tile和b_tile block内的shared memory协作读取A和B的tile，然后计算C[row, col]
__global__ void gemm_tiled_kernel(const float *A, const float *B, float *C,
                                  int M, int N, int K) {
  __shared__ float a_tile[TILE][TILE];
  __shared__ float b_tile[TILE][TILE];

  const int row = blockIdx.y * TILE + threadIdx.y;
  const int col = blockIdx.x * TILE + threadIdx.x;
  const int tile_count = (K + TILE - 1) / TILE;
  float sum = 0.0F;

  for (int tile = 0; tile < tile_count; ++tile) {
    const int a_col = tile * TILE + threadIdx.x;
    const int b_row = tile * TILE + threadIdx.y;

    // 尺寸不是 TILE 的整数倍时，用 0 填充越界部分。
    a_tile[threadIdx.y][threadIdx.x] =
        (row < M && a_col < K) ? A[row * K + a_col] : 0.0F;
    b_tile[threadIdx.y][threadIdx.x] =
        (b_row < K && col < N) ? B[b_row * N + col] : 0.0F;
    __syncthreads();

    for (int k = 0; k < TILE; ++k) {
      sum += a_tile[threadIdx.y][k] * b_tile[k][threadIdx.x];
    }
    __syncthreads();
  }

  if (row < M && col < N) {
    C[row * N + col] = sum;
  }
}

// 启动示例：A、B、C 均为 device pointer。
void launch_gemm_tiled(const float *A, const float *B, float *C, int M, int N,
                       int K) {
  const dim3 block(TILE, TILE);
  const dim3 grid((N + TILE - 1) / TILE, (M + TILE - 1) / TILE);
  gemm_tiled_kernel<<<grid, block>>>(A, B, C, M, N, K);
}
