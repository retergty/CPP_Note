#include <atomic>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cstdint>

// 缓存行大小，用于消除伪共享
#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t CACHELINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr size_t CACHELINE_SIZE = 64;
#endif

class LockFreeByteQueue {
public:
    // 强制要求容量必须是 2 的幂次方
    explicit LockFreeByteQueue(size_t capacity) : write_idx_(0), read_idx_(0) {
        if (capacity == 0 || (capacity & (capacity - 1)) != 0) {
            throw std::invalid_argument("Capacity must be a power of 2");
        }
        capacity_ = capacity;
        mask_ = capacity_ - 1;
        buffer_ = new uint8_t[capacity_];
    }

    ~LockFreeByteQueue() {
        delete[] buffer_;
    }

    LockFreeByteQueue(const LockFreeByteQueue&) = delete;
    LockFreeByteQueue& operator=(const LockFreeByteQueue&) = delete;

    // ==========================================
    // 生产者：写入字节流 (无锁)
    // ==========================================
    size_t write(const uint8_t* data, size_t len) {
        if (len == 0) return 0;

        // Relaxed: 只有生产者自己修改写游标
        size_t current_write = write_idx_.load(std::memory_order_relaxed);
        
        // Acquire: 获取消费者最新的读进度，确保能看到消费者释放的空间
        size_t current_read = read_idx_.load(std::memory_order_acquire);

        // 计算剩余空间
        size_t available_space = capacity_ - (current_write - current_read);
        if (available_space == 0) return 0; // 队列满了

        // 实际写入长度
        size_t write_len = std::min(len, available_space);

        // 计算物理索引
        size_t physical_write_idx = current_write & mask_;
        size_t space_to_end = capacity_ - physical_write_idx;

        // 核心：在锁外尽情地 memcpy！(此时还没有更新全局游标，消费者看不见)
        if (write_len <= space_to_end) {
            std::memcpy(buffer_ + physical_write_idx, data, write_len);
        } else {
            std::memcpy(buffer_ + physical_write_idx, data, space_to_end);
            std::memcpy(buffer_, data + space_to_end, write_len - space_to_end);
        }

        // 保证在 write_idx_ 更新对消费者可见时，上面的 memcpy 数据绝对已经写入了物理内存！
        write_idx_.store(current_write + write_len, std::memory_order_release);

        return write_len;
    }

    // ==========================================
    // 消费者：读取字节流 (无锁)
    // ==========================================
    size_t read(uint8_t* out_data, size_t len) {
        if (len == 0) return 0;

        // Relaxed: 只有消费者自己修改读游标
        size_t current_read = read_idx_.load(std::memory_order_relaxed);
        
        // Acquire: 获取生产者最新的写进度，确保能看到生产者 memcpy 进来的真实数据
        size_t current_write = write_idx_.load(std::memory_order_acquire);

        // 计算积压的数据量
        size_t available_data = current_write - current_read;
        if (available_data == 0) return 0; // 队列空了

        // 实际读取长度
        size_t read_len = std::min(len, available_data);

        // 计算物理索引
        size_t physical_read_idx = current_read & mask_;
        size_t space_to_end = capacity_ - physical_read_idx;

        // 核心：尽情地 memcpy 读取数据！
        if (read_len <= space_to_end) {
            std::memcpy(out_data, buffer_ + physical_read_idx, read_len);
        } else {
            std::memcpy(out_data, buffer_ + physical_read_idx, space_to_end);
            std::memcpy(out_data + space_to_end, buffer_, read_len - space_to_end);
        }

        // 保证在 read_idx_ 更新对生产者可见时，消费者绝对已经把数据读走了，生产者可以安全覆盖那块内存！
        read_idx_.store(current_read + read_len, std::memory_order_release);

        return read_len;
    }

private:
    size_t capacity_;
    size_t mask_;
    uint8_t* buffer_;

    // 缓存行隔离，彻底消灭伪共享 (False Sharing)
    alignas(CACHELINE_SIZE) std::atomic<size_t> write_idx_;
    alignas(CACHELINE_SIZE) std::atomic<size_t> read_idx_;
};
