#include <future>
#include <iostream>

#include <atomic>

template<typename T> 
class BlockingQueue;

template <typename T>
class Producer {
public:
    explicit Producer(size_t capacity) : queue_(capacity), is_stopped_(false) {}

    ~Producer() { stop(); }

    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;

    // 派发消费者
    Consumer<T> create_consumer() {
        std::promise<void> prom;
        consumer_done_future_ = prom.get_future();
        return Consumer<T>(queue_, std::move(prom));
    }

    // 生产数据 (左值和右值转发)
    bool produce(const T& data) { return queue_.push(data); }
    bool produce(T&& data)      { return queue_.push(std::move(data)); }

    void stop() {
        bool expected = false;
        if (!is_stopped_.compare_exchange_strong(expected, true)) return;

        // std::cout << "[Producer] Closing the queue...\n";
        
        queue_.close(); 

        // 步骤 B：死等消费者下线 (物理防御，杜绝段错误)
        if (consumer_done_future_.valid()) {
            consumer_done_future_.wait();
        }
        
        // std::cout << "[Producer] Consumer gracefully exited. Safe to destruct.\n";
    }

private:
    BlockingQueue<T> queue_;
    std::atomic<bool> is_stopped_;
    std::future<void> consumer_done_future_;
};

template <typename T>
class Consumer {
public:
    Consumer(BlockingQueue<T>& queue, std::promise<void> done_promise)
        : queue_(queue), done_promise_(std::move(done_promise)) {}

    Consumer(const Consumer&) = delete;
    Consumer(Consumer&&) = default;

    void run() {
        T data;
        // 只要能取出数据，就一直干。
        // pop() 会在没数据时自动休眠；会在队列关闭且掏空时自动返回 false
        while (queue_.pop(data)) {
            // ... 处理业务数据 ...
            // std::cout << "Processed data\n";
        }

        // 离开房间前，依然履行诺言，触发检票机！
        // std::cout << "[Consumer] Finished all work. Exiting.\n";
        done_promise_.set_value();
    }

private:
    BlockingQueue<T>& queue_;
    std::promise<void> done_promise_;
};
