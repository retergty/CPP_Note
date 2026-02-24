#include <atomic>
#include <cstddef>
#include <type_traits>
#include <new>

#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t CACHELINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr size_t CACHELINE_SIZE = 64;
#endif

template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0), 
                  "Capacity must be a power of 2");
    static constexpr size_t MASK = Capacity - 1;

public:
    SPSCQueue() = default;
    ~SPSCQueue() = default;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // 生产者调用：将元素推入队列
    bool push(const T& item) {
        // Relaxed：生产者自己修改自己的指针，不存在竞争
        const size_t current_write = write_idx_.load(std::memory_order_relaxed);
        
        if (current_write - read_idx_.load(std::memory_order_acquire) >= Capacity) {
            return false; // 队列满了
        }

        // 写入数据
        buffer_[current_write & MASK] = item;
        
        // Release 语义：保证数据写入内存在更新 write_idx_ 之前对消费者可见
        write_idx_.store(current_write + 1, std::memory_order_release);
        return true;
    }

    // 消费者调用：从队列弹出元素
    bool pop(T& item) {
        // Relaxed：消费者自己修改自己的指针，不存在竞争
        const size_t current_read = read_idx_.load(std::memory_order_relaxed);

        if (write_idx_.load(std::memory_order_acquire) == current_read) {
            return false; // 队列空了
        }

        // 读取数据
        item = buffer_[current_read & MASK];
        
        // Release 语义：保证数据读取完成在更新 read_idx_ 之前对生产者可见
        read_idx_.store(current_read + 1, std::memory_order_release);
        return true;
    }

private:
    alignas(CACHELINE_SIZE) std::atomic<size_t> write_idx_{0};
    alignas(CACHELINE_SIZE) std::atomic<size_t> read_idx_{0};

    // 数据区
    alignas(CACHELINE_SIZE) T buffer_[Capacity];
};
