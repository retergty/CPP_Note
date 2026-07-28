#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <utility>

// 无锁的最新值双缓冲，仅支持一个生产者和一个消费者。
// 生产者可以并发写入后台缓冲区；发布失败时，新值留在不可见的后台
// 缓冲区中，消费者仍继续读取原来的前台缓冲区。
template <class T>
class LockFreeDoubleBuffer {
 public:
  static_assert(ATOMIC_INT_LOCK_FREE == 2,
                "unsigned atomics must be lock-free");

  // 写入后台缓冲区，并尝试通过 CAS 发布。
  // 返回 false 表示消费者尚未读完，本次不切换前后台缓冲区。
  template <class U>
  bool TryPublish(U&& value) {
    const unsigned state = state_.load(std::memory_order_acquire);
    const unsigned front = state & kFrontMask;
    const unsigned back = front ^ 1U;

    // 消费者只访问前台缓冲区，因此这里可以并发写入后台缓冲区。
    buffers_[back] = std::forward<U>(value);

    // 仅当前台编号未改变且读取标志已清除时才允许发布。
    unsigned expected = front;
    return state_.compare_exchange_strong(
        expected, back, std::memory_order_release, std::memory_order_relaxed);
  }

  // 尝试取得前台缓冲区的读取所有权。
  // reader 接收到的引用不得保存到回调之外。
  template <class F>
  bool TryRead(F&& reader) {
    unsigned state = state_.load(std::memory_order_relaxed);
    if ((state & kReading) != 0U) {
      return false;
    }

    const unsigned reading_state = state | kReading;
    // CAS 成功后以 acquire 获取已发布的缓冲区；失败时不使用实际状态。
    if (!state_.compare_exchange_strong(state, reading_state,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
      return false;
    }

    const unsigned index = reading_state & kFrontMask;

    ReadGuard guard(*this);
    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[index]));
    return true;
  }

 private:
  // 离开读取作用域时自动清除读取标志。
  class ReadGuard {
   public:
    explicit ReadGuard(LockFreeDoubleBuffer& owner) : owner_(owner) {}

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    ~ReadGuard() { owner_.FinishRead(); }

   private:
    LockFreeDoubleBuffer& owner_;
  };

  // 清除读取标志，使生产者之后可以发布后台缓冲区。
  void FinishRead() noexcept {
    state_.fetch_and(~kReading, std::memory_order_release);
  }

  // state_ 的 bit 0 保存前台缓冲区编号，bit 1 表示消费者正在读取。
  static constexpr unsigned kFrontMask = 0x01U;
  static constexpr unsigned kReading = 0x02U;

  std::array<T, 2> buffers_{};
  std::atomic<unsigned> state_{0U};
};
