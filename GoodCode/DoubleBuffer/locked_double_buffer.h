#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

// 有锁的最新值双缓冲，仅支持一个生产者和一个消费者。
// 生产者可以在消费者读取前台缓冲区时写入后台缓冲区；如果消费者在发布
// 时仍未读完，则不切换前后台缓冲区并返回 false。
template <class T>
class LockedDoubleBuffer {
 public:
  // 写入后台缓冲区，并尝试将其发布为新的前台缓冲区。
  template <class U>
  bool TryPublish(U&& value) {
    // back_ 仅由唯一的生产者访问，因此写入数据时不需要加锁。
    buffers_[back_] = std::forward<U>(value);

    std::lock_guard<std::mutex> lock(state_mutex_);
    // 消费者尚未读完时，后台数据保持不可见并留待下次覆盖。
    if (reading_) {
      return false;
    }

    std::swap(front_, back_);
    return true;
  }

  // 在回调执行期间保持当前前台缓冲区的读取所有权。
  // reader 接收到的引用不得保存到回调之外。
  template <class F>
  bool TryRead(F&& reader) {
    std::size_t index;

    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      if (reading_) {
        return false;
      }

      reading_ = true;
      index = front_;
    }

    ReadGuard guard(*this);
    std::invoke(std::forward<F>(reader),
                static_cast<const T&>(buffers_[index]));
    return true;
  }

 private:
  // 离开读取作用域时自动释放读取所有权。
  // 防止用户回调抛出异常时，无法正确释放读取所有权。
  class ReadGuard {
   public:
    explicit ReadGuard(LockedDoubleBuffer& owner) : owner_(owner) {}

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    ~ReadGuard() { owner_.FinishRead(); }

   private:
    LockedDoubleBuffer& owner_;
  };

  void FinishRead() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    reading_ = false;
  }

  std::array<T, 2> buffers_{};
  std::mutex state_mutex_;
  bool reading_ = false;
  std::size_t front_ = 0;
  // 仅由生产者访问，始终指向当前非前台缓冲区。
  std::size_t back_ = 1;
};
