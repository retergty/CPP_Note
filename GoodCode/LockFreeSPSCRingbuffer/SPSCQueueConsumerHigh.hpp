#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <utility> // for std::move

// 硬件缓存行大小，防止伪共享
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T>
class BlockingSPSCQueue {
public:
    explicit BlockingSPSCQueue(size_t capacity)
        : capacity_(capacity + 1), // 多分配一个空间用于区分空/满
        buffer_(capacity + 1)
    {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
        producer_waiting_.store(false, std::memory_order_relaxed);
    }

    // 禁止拷贝
    BlockingSPSCQueue(const BlockingSPSCQueue&) = delete;
    BlockingSPSCQueue& operator=(const BlockingSPSCQueue&) = delete;

    // ============================================================
    // 生产者接口 (运行在普通线程)
    // ============================================================

    // 尝试写入，如果满了则阻塞等待
    void push(T&& item) {
        // 1. 获取当前写指针 (Relaxed: 只有生产者修改 head)
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (current_head + 1) % capacity_;

        // 2. 检查是否已满 (Acquire: 必须看到消费者更新 tail)
        if (next_head != tail_.load(std::memory_order_acquire)) {
            // --- 快速路径 (Fast Path): 队列未满 ---
            // 直接无锁写入，无需任何 Mutex/CV 参与
            buffer_[current_head] = std::forward<T>(item);
            head_.store(next_head, std::memory_order_release);
            return;
        }

        // --- 慢速路径 (Slow Path): 队列满了 ---
        // 只有在队列满时，才涉及锁的操作
        std::unique_lock<std::mutex> lock(mtx_);

        // 设置标志位：告诉高优先级线程“我因为满了而暂停了”
        producer_waiting_.store(true, std::memory_order_release);

        // 循环等待 (防止虚假唤醒)
        while (true) {
            // 在锁内再次检查 tail，看是否有空间了
            // 注意：这里读取 tail 依然用 Acquire，虽然有锁，但 tail 是原子的
            if (next_head != tail_.load(std::memory_order_acquire)) {
                break; // 有空间了
            }
            // 等待消费者唤醒
            cv_.wait(lock);
        }

        // 醒来后，写入数据
        buffer_[current_head] = std::forward<T>(item);

        // 更新 head
        head_.store(next_head, std::memory_order_release);

        // 清除等待标志
        producer_waiting_.store(false, std::memory_order_relaxed);
    }

    // ============================================================
    // 消费者接口 (运行在高优先级线程)
    // 目标：绝不阻塞，除非必须发送信号否则不碰锁
    // ============================================================

    // 尝试弹出数据，返回 false 表示空
    bool pop(T& item) {
        // 1. 获取当前读指针
        const size_t current_tail = tail_.load(std::memory_order_relaxed);

        // 2. 检查是否为空 (Acquire: 必须看到生产者更新 head)
        const size_t current_head = head_.load(std::memory_order_acquire);

        if (current_tail == current_head) {
            return false; // 队列空，直接返回，让高优先级线程去处理其他任务
        }

        // 3. 读取数据 (无锁)
        item = std::move(buffer_[current_tail]);

        // 4. 计算新的 tail
        const size_t next_tail = (current_tail + 1) % capacity_;

        // 5. 更新 tail (Release: 告诉生产者这个位置空出来了)
        tail_.store(next_tail, std::memory_order_release);

        // ============================================================
        // 信号通知逻辑 (关键优化)
        // ============================================================

        // 检查生产者是否在等待。
        // 如果 producer_waiting_ 为 false，说明生产者在全速运行（没满），
        // 我们不需要去拿锁，直接结束函数。这保证了高优先级线程的性能。
        if (producer_waiting_.load(std::memory_order_acquire)) {

            // 只有发现生产者可能在睡觉，才去拿锁通知
            std::unique_lock<std::mutex> lock(mtx_);

            // Double Check: 再次确认，防止在拿锁过程中生产者已经醒了
            if (producer_waiting_.load(std::memory_order_relaxed)) {
                // 提前清除标志位，防止下一次 pop 又进锁
                producer_waiting_.store(false, std::memory_order_relaxed);
                cv_.notify_one();
            }
        }

        return true;
    }

    // 辅助函数
    bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    size_t capacity() const {
        return capacity_ - 1;
    }

private:
    size_t capacity_;
    std::vector<T> buffer_;

    // -----------------------------------------------------------------
    // 缓存行对齐区：防止伪共享
    // -----------------------------------------------------------------

    alignas(hardware_destructive_interference_size) std::atomic<size_t> head_;
    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_;

    // -----------------------------------------------------------------
    // 信号同步区：仅用于队列满时的阻塞
    // -----------------------------------------------------------------

    // 这里的变量放在一起，因为它们总是被锁保护或一起使用
    alignas(hardware_destructive_interference_size) std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> producer_waiting_; // 优化开关
};
