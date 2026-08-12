#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

// 有锁的最新值三缓冲，仅支持一个生产者和一个消费者。
// 语义同 LockedDoubleBuffer：可读前台时写后台；发布时若尚未读完则不切换
// 前台并返回 false，后台数据保持不可见并留待下次覆盖。相对双缓冲多一
// 个空闲槽，仅在发布成功后轮转；读未完成时发布仍失败。
template <class T>
class LockedTripleBuffer {
 public:
  // 写入后台，并尝试发布为新前台。
  // 返回 false：消费者尚未读完，不切换前台，后台数据保持不可见。
  template <class U>
  bool TryPublish(U&& value) {
    // back_ 仅由唯一生产者访问，写入时无需加锁。
    buffers_[back_] = std::forward<U>(value);

    std::lock_guard<std::mutex> lock(state_mutex_);
    if (reading_) {
      return false;
    }

    // 旧前台进入 middle，原 middle 作为下次写入的 back。
    const std::size_t old_front = front_;
    front_ = back_;
    back_ = middle_;
    middle_ = old_front;
    return true;
  }

  // 回调期间保持前台读取所有权。引用不得保存到回调之外。
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
  // 离开读取作用域时释放读取所有权（含回调抛异常的情况）。
  class ReadGuard {
   public:
    explicit ReadGuard(LockedTripleBuffer& owner) : owner_(owner) {}

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    ~ReadGuard() { owner_.FinishRead(); }

   private:
    LockedTripleBuffer& owner_;
  };

  void FinishRead() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    reading_ = false;
  }

  // 初始：front=0，middle=1，back=2。
  std::array<T, 3> buffers_{};
  std::mutex state_mutex_;
  bool reading_ = false;
  std::size_t front_ = 0;
  std::size_t middle_ = 1;
  // 仅生产者访问，指向当前可写的非前台槽。
  std::size_t back_ = 2;
};
