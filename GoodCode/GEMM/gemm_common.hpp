#pragma once

#include <cstddef>

namespace gemm {
namespace detail {

template <typename T>
void zero_matrix(std::size_t M, std::size_t N, T *C, std::size_t ldc) {
  for (std::size_t i = 0; i < M; ++i) {
    T *c_row = C + i * ldc;
    for (std::size_t j = 0; j < N; ++j) {
      c_row[j] = T{};
    }
  }
}

} // namespace detail
} // namespace gemm
