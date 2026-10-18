#include "cuda_check.cuh"

#include <cstdio>
#include <vector>

__global__ void vector_add(const float* a, const float* b, float* c, int n) {
  const int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) {
    c[i] = a[i] + b[i];
  }
}

int main() {
  int device = 0;
  CUDA_CHECK(cudaGetDevice(&device));

  cudaDeviceProp prop{};
  CUDA_CHECK(cudaGetDeviceProperties(&prop, device));
  std::printf("Device : %s\n", prop.name);
  std::printf("SM     : %d.%d\n", prop.major, prop.minor);
  std::printf("Memory : %zu MiB\n", prop.totalGlobalMem / (1024 * 1024));
  std::printf("CUDA   : %d.%d\n", CUDART_VERSION / 1000,
              (CUDART_VERSION % 1000) / 10);

  constexpr int n = 1 << 20;
  const size_t bytes = static_cast<size_t>(n) * sizeof(float);

  std::vector<float> h_a(n, 1.0f);
  std::vector<float> h_b(n, 2.0f);
  std::vector<float> h_c(n, 0.0f);

  float *d_a = nullptr, *d_b = nullptr, *d_c = nullptr;
  CUDA_CHECK(cudaMalloc(&d_a, bytes));
  CUDA_CHECK(cudaMalloc(&d_b, bytes));
  CUDA_CHECK(cudaMalloc(&d_c, bytes));
  CUDA_CHECK(cudaMemcpy(d_a, h_a.data(), bytes, cudaMemcpyHostToDevice));
  CUDA_CHECK(cudaMemcpy(d_b, h_b.data(), bytes, cudaMemcpyHostToDevice));

  constexpr int threads = 256;
  const int blocks = (n + threads - 1) / threads;
  vector_add<<<blocks, threads>>>(d_a, d_b, d_c, n);
  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaDeviceSynchronize());
  CUDA_CHECK(cudaMemcpy(h_c.data(), d_c, bytes, cudaMemcpyDeviceToHost));

  bool ok = true;
  for (int i = 0; i < n; ++i) {
    if (h_c[i] != 3.0f) {
      ok = false;
      break;
    }
  }
  std::printf("vector_add: %s\n", ok ? "OK" : "FAILED");

  CUDA_CHECK(cudaFree(d_a));
  CUDA_CHECK(cudaFree(d_b));
  CUDA_CHECK(cudaFree(d_c));
  return ok ? 0 : 1;
}
