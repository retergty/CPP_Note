#pragma once

#include <vector>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <thread>

// 硬件缓存行大小
#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_destructive_interference_size;
#else
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T>
class LockFreeOverwriteRingBuffer {
public:
    // 容量建议是 2 的幂，虽然这里用了取模，但为了位运算优化可以改
    explicit LockFreeOverwriteRingBuffer(size_t capacity) 
        : buffer_(capacity), capacity_(capacity) {}

    // ==========================================================
    // 生产者：高优先级 / ISR
    // 特点：Wait-Free，绝不阻塞，绝不循环，无视 Full 状态直接覆盖
    // ==========================================================
    void push(const T& data) {
        // 1. 获取当前写入位置 (单调递增的索引)
        uint64_t current_head = head_.load(std::memory_order_relaxed);
        
        // 2. 写入数据
        // 注意：这里可能会覆盖消费者正在读的数据，但这没关系，消费者会检测到
        buffer_[current_head % capacity_] = data;

        // 3. 更新 Head (Release)
        // 这一步是“提交”操作。一旦 head 更新，消费者就知道有新数据了
        head_.store(current_head + 1, std::memory_order_release);
    }

    // ==========================================================
    // 消费者：普通线程
    // 特点：乐观读取。如果读取过程中发现数据被覆盖了，就重试。
    // ==========================================================
    bool pop(T& out_data) {
        uint64_t current_tail = tail_; // 本地缓存的 tail
        uint64_t current_head;

        // 乐观读取循环 (Optimistic Loop)
        do {
            // A. 获取当前的 head (Acquire)
            // 建立了内存屏障，保证之后的读操作能看到生产者之前的写操作
            current_head = head_.load(std::memory_order_acquire);

            // 检查是否为空
            if (current_tail == current_head) {
                return false; 
            }

            // B. 处理“被覆盖”的情况 (Overrun / Overflow)
            // 如果 head 跑得太快，已经绕了一圈超过了 tail，
            // 说明 tail 指向的数据已经被覆盖了，我们必须跳过旧数据，追上进度。
            if (current_head - current_tail > capacity_) {
                // 直接把 tail 瞬移到最新的有效数据位置
                // 例如：head=100, cap=10, tail=50 -> tail 设为 90 (head - cap)
                current_tail = current_head - capacity_;
            }

            // C. 尝试拷贝数据
            // 此时可能发生竞争：我们在拷贝的时候，生产者可能正好又把这个位置覆盖了
            out_data = buffer_[current_tail % capacity_];

            // D. 二次检查 (Double Check) - 核心逻辑
            // 再次读取 head。如果 head 在我们拷贝数据的过程中变了，
            // 且变的幅度大到覆盖了我们刚才读的位置，说明我们读到的是“脏数据” (Torn Read)。
            
            // 获取屏障，防止指令重排
            std::atomic_thread_fence(std::memory_order_acquire); 
            uint64_t head_after = head_.load(std::memory_order_relaxed);

            // 检查：在我们读取期间，生产者是否覆盖了当前 tail 的位置？
            // 只要 head_after 依然保持在 "tail + capacity" 范围内，说明 tail 还没被覆盖。
            // 如果 head_after - current_tail > capacity_，说明生产者绕了一圈回来了。
            if (head_after - current_tail <= capacity_) {
                // 读取成功，有效！
                tail_ = current_tail + 1; // 更新本地 tail
                return true;
            }

            // 如果走到这里，说明发生了由于覆盖导致的数据撕裂。
            // 策略：不更新 tail，直接重试 (continue loop)，
            // 下一次循环开头会重新计算 current_tail = current_head - capacity_
            
        } while (true);
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;

    // 只要 head，不需要原子的 tail (tail 由消费者私有维护)
    alignas(hardware_destructive_interference_size) std::atomic<uint64_t> head_{0};
    
    // 消费者的私有 tail，不需要 atomic，因为只有消费者自己改
    alignas(hardware_destructive_interference_size) uint64_t tail_{0};
};
