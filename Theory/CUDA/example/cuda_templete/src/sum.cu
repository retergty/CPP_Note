#include <algorithm>
#include <cfloat>
#include <cuda_runtime.h>

__device__ float warp_sum(float value) {
  for (int offset = 16; offset > 0; offset >>= 1) {
    value += __shfl_down_sync(0xFFFFFFFF, value, offset);
  }
  return value;
}

__device__ float block_sum(float value) {
  __shared__ float values[32];

  int warp = threadIdx.x >> 5;
  int lane = threadIdx.x & 0x1F;

  value = warp_sum(value);

  if (lane == 0) {
    values[warp] = value;
  }

  __syncthreads();

  if (warp == 0) {
    int warp_nums = (blockDim.x + 31) >> 5;
    value = (threadIdx.x < warp_nums) ? values[lane] : 0;
    value = warp_sum(value);
  }
  return value;
}

__global__ void sum_kernel(const float *__restrict__ source,
                           float *__restrict__ dest, int n) {
  int grid_stride = blockDim.x * gridDim.x;

  float value = 0;

  int idx = threadIdx.x + blockIdx.x * blockDim.x;

  for (int i = idx; i < n; i += grid_stride) {
    value += source[i];
  }

  value = block_sum(value);

  if (threadIdx.x == 0) {
    dest[blockIdx.x] = value;
  }

  return;
}

void LaunchSumKernel(const float *__restrict__ source, float *__restrict__ dest,
                     int n) {
  if (n <= 0)
    return;
  
  int block_threads = 256;
  int blocks = (n + block_threads - 1) / block_threads;
  blocks = std::min(blocks, 1024);
  float *partial;
  if (cudaMalloc(&partial, blocks * sizeof(float)) != cudaSuccess) {
    return;
  }

  sum_kernel<<<blocks, block_threads>>>(source, partial, n);

  sum_kernel<<<1, block_threads>>>(partial, dest, blocks);

  cudaFree(partial);
  return;
}