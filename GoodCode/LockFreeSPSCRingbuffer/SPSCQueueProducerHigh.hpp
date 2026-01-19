#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility> // for std::move

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T>
class RealtimeProducerQueue {
public:
  explicit RealtimeProducerQueue(size_t capacity)
    : capacity_(capacity + 1),
    buffer_(capacity + 1)
  {
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
    consumer_waiting_.store(false, std::memory_order_relaxed);
  }

  // ============================================================
  // 生产者接口 (高优先级 / 实时线程 / ISR)
  // 目标：绝不阻塞，尽可能不碰锁
  // ============================================================
  bool push(T&& item) {
    const size_t current_head = head_.load(std::memory_order_relaxed);
    const size_t next_head = (current_head + 1) % capacity_;

    // 1. 检查队列是否已满 (Acquire)
    if (next_head == tail_.load(std::memory_order_acquire)) {
      return false; // 队列满，实时线程通常选择丢包或覆盖，立即返回
    }

    // 2. 无锁写入数据
    buffer_[current_head] = std::forward<T>(item);

    // 3. 提交数据 (Release)
    // 这一步之后，消费者就能看到数据了
    head_.store(next_head, std::memory_order_release);

    // ============================================================
    // 信号通知逻辑
    // ============================================================

    // 4. 关键优化：先检查原子标记
    // 如果 consumer_waiting_ 为 false，说明消费者正在忙碌（没在睡觉），
    // 既然它醒着，它处理完当前数据自然会去检查下一个。
    // 我们不需要拿锁，也不需要通知。直接返回！(Zero System Call)
    if (consumer_waiting_.load(std::memory_order_acquire)) {

      std::lock_guard<std::mutex> lock(mtx_);

      // Double Check: 防止在拿锁间隙消费者已经醒了
      if (consumer_waiting_.load(std::memory_order_relaxed)) {
        consumer_waiting_.store(false, std::memory_order_relaxed); // 重置
        cv_.notify_one();
      }
    }

    return true;
  }

  // ============================================================
  // 消费者接口 (普通线程 / 允许阻塞)
  // ============================================================
  void pop(T& item) {
    // 1. 快速路径：先尝试无锁读取
    // 如果队列有数据，直接拿走，完全不涉及锁和 CV
    if (try_pop_lock_free(item)) {
      return;
    }

    // 2. 慢速路径：队列空了，必须加锁等待
    std::unique_lock<std::mutex> lock(mtx_);

    // 标记：告诉生产者“我要去睡觉了，请记得叫我”
    consumer_waiting_.store(true, std::memory_order_release);

    // 3. 循环等待 (标准 CV 用法)
    // 注意：在 wait 之前必须再次检查 try_pop (防止在加锁期间生产者推入了数据)
    while (!try_pop_lock_free(item)) {
      cv_.wait(lock);

      if (empty()) {
        consumer_waiting_.store(true, std::memory_order_relaxed);
      }
    }

    // 成功读取数据，且循环结束
    consumer_waiting_.store(false, std::memory_order_relaxed);
  }

private:
  // 内部使用的无锁 pop，不处理等待逻辑
  bool try_pop_lock_free(T& item) {
    const size_t current_tail = tail_.load(std::memory_order_relaxed);
    const size_t current_head = head_.load(std::memory_order_acquire);

    if (current_tail == current_head) {
      return false;
    }

    item = std::move(buffer_[current_tail]);
    const size_t next_tail = (current_tail + 1) % capacity_;
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  bool empty() const {
    return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
  }

private:
  size_t capacity_;
  std::vector<T> buffer_;

  alignas(hardware_destructive_interference_size) std::atomic<size_t> head_;
  alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_;

  // 信号同步变量
  alignas(hardware_destructive_interference_size) std::mutex mtx_;
  std::condition_variable cv_;
  std::atomic<bool> consumer_waiting_; // 关键优化标记
};
