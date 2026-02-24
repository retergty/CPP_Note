#include <mutex>
#include <condition_variable>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cstdint>

class BlockingByteQueue {
public:
    explicit BlockingByteQueue(size_t capacity) 
        : write_idx_(0), read_idx_(0), is_closed_(false) 
    {
        if (capacity == 0 || (capacity & (capacity - 1)) != 0) {
            throw std::invalid_argument("Capacity must be a power of 2");
        }
        capacity_ = capacity;
        mask_ = capacity_ - 1;
        buffer_ = new uint8_t[capacity_];
    }

    ~BlockingByteQueue() {
        delete[] buffer_;
    }

    BlockingByteQueue(const BlockingByteQueue&) = delete;
    BlockingByteQueue& operator=(const BlockingByteQueue&) = delete;

    // ==========================================
    // 写入字节流 (返回实际写入的字节数)
    // ==========================================
    size_t write(const uint8_t* data, size_t len) {
        if (len == 0) return 0;

        std::unique_lock<std::mutex> lock(mutex_);
        
        // 阻塞等待：只要有空闲空间，或者已经打烊，就唤醒
        cv_not_full_.wait(lock, [this]() { 
            return (write_idx_ - read_idx_) < capacity_ || is_closed_; 
        });

        if (is_closed_) return 0; // closed,拒绝写入

        // 1. 计算当前真实可用的空闲空间
        size_t available_space = capacity_ - (write_idx_ - read_idx_);
        
        // 2. 实际能写入的长度 (取 min，可能会发生部分写入)
        size_t write_len = std::min(len, available_space);

        // 3. 计算物理写入索引起点，以及到数组末尾的连续空间
        size_t physical_write_idx = write_idx_ & mask_;
        size_t space_to_end = capacity_ - physical_write_idx;

        // 4. 核心截断逻辑 (两次 memcpy)
        if (write_len <= space_to_end) {
            // 情况 A：连续空间足够，一次搞定
            std::memcpy(buffer_ + physical_write_idx, data, write_len);
        } else {
            // 情况 B：空间不够，发生回绕 (Wrap-around)
            std::memcpy(buffer_ + physical_write_idx, data, space_to_end);
            std::memcpy(buffer_, data + space_to_end, write_len - space_to_end);
        }

        // 5. 游标自由狂奔
        write_idx_ += write_len;

        cv_not_empty_.notify_one();
        return write_len;
    }

    // ==========================================
    // 读取字节流 (返回实际读取的字节数)
    // ==========================================
    size_t read(uint8_t* out_data, size_t len) {
        if (len == 0) return 0;

        std::unique_lock<std::mutex> lock(mutex_);
        
        // 阻塞等待：只要有数据，或者已经打烊，就唤醒
        cv_not_empty_.wait(lock, [this]() { 
            return write_idx_ > read_idx_ || is_closed_; 
        });

        // closed且数据被彻底掏空 (EOF)
        if (is_closed_ && write_idx_ == read_idx_) return 0;

        // 1. 计算当前实际积压的数据量
        size_t available_data = write_idx_ - read_idx_;
        
        // 2. 实际能读取的长度
        size_t read_len = std::min(len, available_data);

        // 3. 计算物理读取索引起点，以及到数组末尾的连续空间
        size_t physical_read_idx = read_idx_ & mask_;
        size_t space_to_end = capacity_ - physical_read_idx;

        // 4. 核心截断逻辑 (两次 memcpy)
        if (read_len <= space_to_end) {
            std::memcpy(out_data, buffer_ + physical_read_idx, read_len);
        } else {
            std::memcpy(out_data, buffer_ + physical_read_idx, space_to_end);
            std::memcpy(out_data + space_to_end, buffer_, read_len - space_to_end);
        }

        // 5. 游标自由狂奔
        read_idx_ += read_len;

        cv_not_full_.notify_one();
        return read_len;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_closed_ = true;
        }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

private:
    size_t capacity_;
    size_t mask_;
    uint8_t* buffer_;
    
    size_t write_idx_;
    size_t read_idx_; 
    
    bool is_closed_;
    std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
};
