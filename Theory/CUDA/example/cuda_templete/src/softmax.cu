#include <cuda_runtime.h>

__device__ float WarpMax(float value);

__device__ float BlockMax(float value);

__device__ void MaxKernel(const float *__restrict__ source,
                          float *__restrict__ dest, std::size_t dimision,
                          std::size_t inner);

__device__ float WarpSum(float value);

__device__ float BlockSum(float value);