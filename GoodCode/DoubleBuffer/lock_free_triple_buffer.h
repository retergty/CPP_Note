#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <utility>

// 无锁的最新值三缓冲，仅支持一个生产者和一个消费者。
// front 由消费者独占，back 由生产者独占，middle 用于原子交接。
// 当消费者速度较慢时，中间更新可能被覆盖，但生产者不会被消费者阻塞。
template <class T>
class LockFreeTripleBuffer {
 public:
  static_assert(ATOMIC_INT_LOCK_FREE == 2,
                "unsigned atomics must be lock-free");

  // 仅允许生产者线程调用。写完 back 后，将其发布到 middle，并把原来的
  // middle 取回作为下一次写入使用的 back。
  template <class U>
  void Publish(U&& value) {
    buffers_[back_] = std::forward<U>(value);

    const unsigned previous =
        middle_.exchange(back_ | kDirty, std::memory_order_acq_rel);

    back_ = previous & kIndexMask;
  }

  // 仅允许消费者线程调用。成功时取得最新发布的缓冲区并执行 reader；
  // 自上次成功读取后没有新数据时返回 false。
  // reader 接收到的引用不得保存到回调之外。
  template <class F>
  bool TryReadLatest(F&& reader) {
    const unsigned state = middle_.load(std::memory_order_relaxed);
    if ((state & kDirty) == 0U) {
      return false;
    }

    // 将旧 front 交还给 middle，同时取得生产者最新发布的缓冲区。
    const unsigned previous =
        middle_.exchange(front_, std::memory_order_acq_rel);

    front_ = previous & kIndexMask;

    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[front_]));

    return true;
  }

 private:
  // middle_ 的低两位保存缓冲区编号，bit 2 表示存在尚未读取的新数据。
  static constexpr unsigned kIndexMask = 0x03U;
  static constexpr unsigned kDirty = 0x04U;

  // 初始所有权：front = 0、middle = 1、back = 2。
  std::array<T, 3> buffers_{};
  std::atomic<unsigned> middle_{1U};
  unsigned back_ = 2U;
  unsigned front_ = 0U;
};
