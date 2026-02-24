#include <mutex>
#include <condition_variable>
#include <utility>
#include <stdexcept>

template <typename T>
class BlockingRingBuffer {
public:
    // 构造函数：强校验容量必须是 2 的幂次方
    explicit BlockingRingBuffer(size_t capacity) 
        : capacity_(capacity), read_idx_(0), write_idx_(0), is_closed_(false) 
    {
        if (capacity_ == 0 || (capacity_ & (capacity_ - 1)) != 0) {
            throw std::invalid_argument("Capacity must be a power of 2");
        }
        
        mask_ = capacity_ - 1;       // 例如：capacity 1024 (10000000000)，mask 就是 1023 (01111111111)
        buffer_ = new T[capacity_];  // 一次性分配连续的裸数组
    }

    ~BlockingRingBuffer() {
        delete[] buffer_; 
    }

    BlockingRingBuffer(const BlockingRingBuffer&) = delete;
    BlockingRingBuffer& operator=(const BlockingRingBuffer&) = delete;

    // ==========================================
    // 生产端逻辑
    // ==========================================
    
    // 左值推入
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_not_full_.wait(lock, [this]() { 
            return (write_idx_ - read_idx_) < capacity_ || is_closed_; 
        });
        
        if (is_closed_) return false;

        buffer_[write_idx_ & mask_] = item;
        
        write_idx_++; 

        cv_not_empty_.notify_one();
        return true;
    }

    // 右值推入 (极致零拷贝)
    bool push(T&& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_not_full_.wait(lock, [this]() { 
            return (write_idx_ - read_idx_) < capacity_ || is_closed_; 
        });
        
        if (is_closed_) return false;

        buffer_[write_idx_ & mask_] = std::move(item);
        write_idx_++;

        cv_not_empty_.notify_one();
        return true;
    }

    // ==========================================
    // 消费端逻辑
    // ==========================================
    
    // 弹出数据
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_not_empty_.wait(lock, [this]() { 
            return write_idx_ > read_idx_ || is_closed_; 
        });

        if (is_closed_ && write_idx_ == read_idx_) return false; 

        // 读取数据：所有权转移出队列
        item = std::move(buffer_[read_idx_ & mask_]);
        
        read_idx_++;

        cv_not_full_.notify_one();
        return true;
    }

    // ==========================================
    // 系统控制
    // ==========================================
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_closed_ = true;
        }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

private:
    size_t capacity_;   // 容量 (必须为 2^N)
    size_t mask_;       // 掩码 (capacity - 1)
    T* buffer_;         // 连续的堆内存块
    
    size_t read_idx_;       // 自由狂奔的读游标
    size_t write_idx_;       // 自由狂奔的写游标
    
    bool is_closed_;
    std::mutex mutex_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
};
