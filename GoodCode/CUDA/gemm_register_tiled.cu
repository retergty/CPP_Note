/*
 * CUDA GEMM：共享内存分块 + 一维寄存器分块
 *
 * 计算：
 *   C[M, N] = A[M, K] * B[K, N]
 *
 * 存储：
 *   矩阵均为 row-major。
 *
 * 相比基础 Tiled GEMM：
 *   基础版本中，一个线程只计算 C 的一个元素。
 *   本版本中，一个线程计算同一列上的 THREAD_M 个元素，并将中间结果
 *   保存在 accum[] 寄存器中。
 *
 * 分块方式：
 *   一个 block 计算 C 的 BLOCK_M × BLOCK_N 子块。
 *   A 的 BLOCK_M × BLOCK_K 子块和 B 的 BLOCK_K × BLOCK_N 子块
 *   由整个 block 协作加载到 shared memory。
 *   每个线程负责 THREAD_M × 1 个输出元素。
 *
 * 主要收益：
 *   从 shared memory 读取一次 B 的值后，可以用于 THREAD_M 次乘加，
 *   提高数据复用率和单线程指令级并行度。
 */

#include <cuda_runtime.h>

constexpr int BLOCK_M = 64;
constexpr int BLOCK_N = 32;
constexpr int BLOCK_K = 8;
constexpr int THREAD_M = 8;
constexpr int THREADS_PER_BLOCK = BLOCK_M * BLOCK_N / THREAD_M;  // 256

__global__ void gemm_register_tiled_kernel(const float *A, const float *B,
                                           float *C, int M, int N, int K) {
  __shared__ float a_tile[BLOCK_M][BLOCK_K];
  __shared__ float b_tile[BLOCK_K][BLOCK_N];

  const int tid = threadIdx.x;
  const int thread_row = tid / BLOCK_N;
  const int thread_col = tid % BLOCK_N;
  const int block_row = blockIdx.y * BLOCK_M;
  const int block_col = blockIdx.x * BLOCK_N;

  // 每个线程的 THREAD_M 个累加结果均保存在寄存器中。
  float accum[THREAD_M] = {0.0F};
  const int tile_count = (K + BLOCK_K - 1) / BLOCK_K;

  for (int tile = 0; tile < tile_count; ++tile) {
    // 整个 block 协作加载 A 的子块。
    for (int index = tid; index < BLOCK_M * BLOCK_K;
         index += THREADS_PER_BLOCK) {
      const int local_row = index / BLOCK_K;
      const int local_k = index % BLOCK_K;
      const int global_row = block_row + local_row;
      const int global_k = tile * BLOCK_K + local_k;

      a_tile[local_row][local_k] =
          (global_row < M && global_k < K)
              ? A[global_row * K + global_k]
              : 0.0F;
    }

    // 整个 block 协作加载 B 的子块。
    for (int index = tid; index < BLOCK_K * BLOCK_N;
         index += THREADS_PER_BLOCK) {
      const int local_k = index / BLOCK_N;
      const int local_col = index % BLOCK_N;
      const int global_k = tile * BLOCK_K + local_k;
      const int global_col = block_col + local_col;

      b_tile[local_k][local_col] =
          (global_k < K && global_col < N)
              ? B[global_k * N + global_col]
              : 0.0F;
    }
    __syncthreads();

    for (int k = 0; k < BLOCK_K; ++k) {
      const float b_value = b_tile[k][thread_col];

      for (int i = 0; i < THREAD_M; ++i) {
        const int local_row = thread_row * THREAD_M + i;
        accum[i] += a_tile[local_row][k] * b_value;
      }
    }
    __syncthreads();
  }

  const int global_col = block_col + thread_col;

  for (int i = 0; i < THREAD_M; ++i) {
    const int global_row = block_row + thread_row * THREAD_M + i;
    if (global_row < M && global_col < N) {
      C[global_row * N + global_col] = accum[i];
    }
  }
}

// 启动示例：A、B、C 均为 device pointer。
void launch_gemm_register_tiled(const float *A, const float *B, float *C, int M,
                                int N, int K) {
  const dim3 block(THREADS_PER_BLOCK);
  const dim3 grid((N + BLOCK_N - 1) / BLOCK_N,
                  (M + BLOCK_M - 1) / BLOCK_M);
  gemm_register_tiled_kernel<<<grid, block>>>(A, B, C, M, N, K);
}
