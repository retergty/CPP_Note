#pragma once

#include <functional>
#include <mutex>
#include <utility>

#include "lock_free_latch_latest_state_register.h"

// 有锁的最新状态寄存器（latch 实现）：写线程经互斥串行化，读线程不取锁。
// 任意数量写者可 Publish，任意数量读者可 Load/Read。
template <class T>
class LockedLatchLatestStateRegister {
 public:
  LockedLatchLatestStateRegister() noexcept = default;

  explicit LockedLatchLatestStateRegister(const T& initial_value) noexcept
      : value_(initial_value) {}

  // 底层写路径假定写者互斥，不能交错 Publish。
  void Publish(const T& value) {
    std::lock_guard<std::mutex> lock(writer_mutex_);
    value_.Publish(value);
  }

  T Load() const noexcept { return value_.Load(); }

  template <class F>
  void Read(F&& reader) const {
    value_.Read(std::forward<F>(reader));
  }

 private:
  LockFreeLatchLatestStateRegister<T> value_;
  std::mutex writer_mutex_;
};
