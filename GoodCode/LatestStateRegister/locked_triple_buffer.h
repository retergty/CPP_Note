#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

// 有锁的最新状态寄存器（三缓冲实现），仅支持一个生产者和一个消费者。
// front 归消费者，back 归生产者，middle 在锁下交接。
// Publish 始终成功，不等待读取；Read 始终成功，可反复读当前最新，
// 以引用访问且不拷贝 T。读方较慢时，尚未取走的中间更新可能被覆盖。
// 对比 DoubleBuffer 三缓冲：后者为 TryPublish/TryRead，读占用时发布失败。
template <class T>
class LockedLatestStateRegister {
 public:
  // 写 back，与 middle 交换，并标记有新数据。
  template <class U>
  void Publish(U&& value) {
    buffers_[back_] = std::forward<U>(value);

    std::lock_guard<std::mutex> lock(state_mutex_);
    std::swap(back_, middle_);
    dirty_ = true;
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
  // 有 dirty 则与 middle 交接并返回 true；否则返回 false。
  // 返回后 front_ 仅消费者访问，读回调期间无需再持锁。
  bool RefreshFront() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!dirty_) {
      return false;
    }

    std::swap(front_, middle_);
    dirty_ = false;
    return true;
  }

  // 初始：front=0，middle=1，back=2。
  std::array<T, 3> buffers_{};
  std::mutex state_mutex_;
  bool dirty_ = false;
  std::size_t front_ = 0;
  std::size_t middle_ = 1;
  // 仅生产者访问。
  std::size_t back_ = 2;
};
