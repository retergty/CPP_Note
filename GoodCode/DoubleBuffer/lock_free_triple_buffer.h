#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <utility>

// 无锁的最新值三缓冲，仅支持一个生产者和一个消费者。
// 语义同 LockFreeDoubleBuffer：可并发写后台；发布失败时新值留在不可见的
// 后台并留待下次覆盖，消费者继续读原前台。相对双缓冲多一个空闲槽，
// 仅在发布成功后轮转；读未完成时发布仍失败。
template <class T>
class LockFreeTripleBuffer {
 public:
  static_assert(ATOMIC_INT_LOCK_FREE == 2,
                "unsigned atomics must be lock-free");

  // 写入后台，并尝试经 CAS 发布为新前台。
  // 返回 false：消费者尚未读完，不切换前台，后台数据保持不可见。
  template <class U>
  bool TryPublish(U&& value) {
    buffers_[back_] = std::forward<U>(value);

    const unsigned state = state_.load(std::memory_order_acquire);
    const unsigned front = state & kIndexMask;

    // 仅当前台未变且读取标志已清除时才允许发布。
    unsigned expected = front;
    if (!state_.compare_exchange_strong(expected, back_,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
      return false;
    }

    // 旧前台进入 middle，原 middle 作为下次写入的 back。
    back_ = middle_;
    middle_ = front;
    return true;
  }

  // 尝试取得前台读取所有权。引用不得保存到回调之外。
  template <class F>
  bool TryRead(F&& reader) {
    unsigned state = state_.load(std::memory_order_relaxed);
    if ((state & kReading) != 0U) {
      return false;
    }

    const unsigned reading_state = state | kReading;
    if (!state_.compare_exchange_strong(state, reading_state,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
      return false;
    }

    const unsigned index = reading_state & kIndexMask;

    ReadGuard guard(*this);
    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[index]));
    return true;
  }

 private:
  // 离开读取作用域时清除读取标志。
  class ReadGuard {
   public:
    explicit ReadGuard(LockFreeTripleBuffer& owner) : owner_(owner) {}

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    ~ReadGuard() { owner_.FinishRead(); }

   private:
    LockFreeTripleBuffer& owner_;
  };

  void FinishRead() noexcept {
    state_.fetch_and(~kReading, std::memory_order_release);
  }

  // state_：低 2 位为前台编号，bit 2 为正在读取。
  static constexpr unsigned kIndexMask = 0x03U;
  static constexpr unsigned kReading = 0x04U;

  // 初始：front=0（存于 state_），middle=1，back=2。
  std::array<T, 3> buffers_{};
  std::atomic<unsigned> state_{0U};
  unsigned middle_ = 1U;
  // 仅生产者访问，指向当前可写的非前台槽。
  unsigned back_ = 2U;
};
