#include <cfloat>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
__device__ float warp_max(float value) {
  for (int offset = 16; offset > 0; offset >>= 1) {
    value = fmaxf(value, __shfl_down_sync(0xFFFFFFFF, value, offset));
  }
  return value;
}

__device__ float block_max(float value) {
  __shared__ float values[32];

  int warp = threadIdx.x >> 5;
  int lane = threadIdx.x & 0x1F;

  value = warp_max(value);

  if (lane == 0) {
    values[warp] = value;
  }

  __syncthreads();

  if (warp == 0) {
    int warps = (blockDim.x + 31) >> 5;
    value = (threadIdx.x < warps) ? values[lane] : -FLT_MAX;
    value = warp_max(value);
  }

  return value;
}

__global__ void max_kernel(const float *__restrict__ source,
                           float *__restrict__ dest, int n) {

  int grids_stride = gridDim.x * blockDim.x;

  float value = -FLT_MAX;

  int idx = threadIdx.x + blockDim.x * blockIdx.x;

  for (int i = idx; i < n; i += grids_stride) {
    value = fmaxf(value, source[i]);
  }

  value = block_max(value);

  if (threadIdx.x == 0) {
    dest[blockIdx.x] = value;
  }
}

void LaunchMaxKernel(const float *__restrict__ source, float *__restrict__ dest,
                     int n) {

  if (n <= 0)
    return;

  int blocks_threads = 256;
  int grid = (n + blocks_threads - 1) / blocks_threads;

  if (grid > 1024)
    grid = 1024;

  float *partial;
  if (cudaMalloc(&partial, sizeof(float) * grid) != cudaSuccess) {
    return;
  };
  max_kernel<<<grid, blocks_threads>>>(source, partial, n);

  max_kernel<<<1, blocks_threads>>>(partial, dest, grid);
  cudaFree(partial);
}