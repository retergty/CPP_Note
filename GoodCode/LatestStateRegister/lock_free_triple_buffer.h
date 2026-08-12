#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <utility>

// 无锁的最新状态寄存器（三缓冲实现），仅支持一个生产者和一个消费者。
// front 归消费者，back 归生产者，middle 做原子交接。
// Publish 始终成功，不等待读取；Read 始终成功，可反复读当前最新，
// 以引用访问且不拷贝 T。读方较慢时，尚未取走的中间更新可能被覆盖。
// 对比 DoubleBuffer 三缓冲：后者为 TryPublish/TryRead，读占用时发布失败。
template <class T>
class LockFreeLatestStateRegister {
 public:
  static_assert(ATOMIC_INT_LOCK_FREE == 2,
                "unsigned atomics must be lock-free");

  // 写 back，发布到 middle，并取回原 middle 作为下次 back。
  template <class U>
  void Publish(U&& value) {
    buffers_[back_] = std::forward<U>(value);

    const unsigned previous =
        middle_.exchange(back_ | kDirty, std::memory_order_acq_rel);

    back_ = previous & kIndexMask;
  }

  // 有新数据则刷新 front，再执行 reader；无新数据仍读当前 front。
  // 引用不得保存到回调之外。
  template <class F>
  void Read(F&& reader) {
    RefreshFront();
    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[front_]));
  }

  // 仅当自上次刷新后确有新发布时才执行 reader，并返回 true。
  template <class F>
  bool TryReadLatest(F&& reader) {
    if (!RefreshFront()) {
      return false;
    }

    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[front_]));
    return true;
  }

 private:
  // middle 带 dirty 则与 front 交接并返回 true；否则返回 false。
  bool RefreshFront() {
    const unsigned state = middle_.load(std::memory_order_relaxed);
    if ((state & kDirty) == 0U) {
      return false;
    }

    const unsigned previous =
        middle_.exchange(front_, std::memory_order_acq_rel);
    front_ = previous & kIndexMask;
    return true;
  }

  // middle_：低 2 位为缓冲区编号，bit 2 表示有尚未读取的新数据。
  static constexpr unsigned kIndexMask = 0x03U;
  static constexpr unsigned kDirty = 0x04U;

  // 初始：front=0，middle=1，back=2。
  std::array<T, 3> buffers_{};
  std::atomic<unsigned> middle_{1U};
  // 仅生产者访问。
  unsigned back_ = 2U;
  unsigned front_ = 0U;
};
