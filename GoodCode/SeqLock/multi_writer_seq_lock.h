#pragma once

#include <mutex>

#include "single_writer_seq_lock.h"

// 多写者 SeqLock：写线程通过互斥锁串行化，读线程不获取互斥锁。
// 任意数量的写线程可以调用 Store，任意数量的读线程可以调用 Load。
template <class T>
class MultiWriterSeqLock {
 public:
  MultiWriterSeqLock() noexcept = default;

  explicit MultiWriterSeqLock(const T& initial_value) noexcept
      : value_(initial_value) {}

  // SeqLock 的奇偶协议要求写者之间不能交错执行。
  void Store(const T& value) {
    std::lock_guard<std::mutex> lock(writer_mutex_);
    value_.Store(value);
  }

  // 读侧不加锁；与写入冲突时由内部 SeqLock 自动重试。
  T Load() const noexcept { return value_.Load(); }

 private:
  SingleWriterSeqLock<T> value_;
  std::mutex writer_mutex_;
};
