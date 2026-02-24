#include <atomic>
#include <thread>
#include <iostream>
#include <future>
#include <string>

// 底层泛型队列声明
template <typename T, size_t Capacity> class SPSCQueue; 

// ==========================================
// 1. 泛型消费者类
// ==========================================
template <typename T, size_t Capacity>
class Consumer {
public:
    Consumer(SPSCQueue<T, Capacity>& queue, 
             std::atomic<bool>& stop_flag, 
             std::promise<void> done_promise)
        : queue_(queue), 
          stop_flag_(stop_flag), 
          done_promise_(std::move(done_promise)) 
    {}

    Consumer(const Consumer&) = delete;
    Consumer(Consumer&&) = default;

    void run() {
        T data; // 泛型数据接收变量
        while (true) {
            if (queue_.pop(data)) {
                // 正常消费 (实际业务中这里可以是一个回调函数或者直接处理)
                // std::cout << "Consumed\n"; 
            } else {
                if (stop_flag_.load(std::memory_order_acquire)) {
                    // 榨干队列残留
                    while (queue_.pop(data)) {}
                    break; 
                }
                std::this_thread::yield();
            }
        }
        // 履行诺言，唤醒老板
        done_promise_.set_value();
    }

private:
    SPSCQueue<T, Capacity>& queue_;
    std::atomic<bool>& stop_flag_;
    std::promise<void> done_promise_;
};

// ==========================================
// 2. 泛型生产者类
// ==========================================
template <typename T, size_t Capacity>
class Producer {
public:
    Producer() : stop_flag_(false), is_stopped_(false) {}

    ~Producer() { stop(); }

    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;

    // 返回泛型的 Consumer
    Consumer<T, Capacity> create_consumer() {
        std::promise<void> prom;
        consumer_done_future_ = prom.get_future();
        return Consumer<T, Capacity>(queue_, stop_flag_, std::move(prom)); 
    }

    void produce(const T& data) {
        while (!queue_.push(data)) {
            std::this_thread::yield();
        }
    }

    void produce(T&& data) {
        while (!queue_.push(std::forward<T>(data))) {
            std::this_thread::yield();
        }
    }

    void stop() {
        bool expected = false;
        if (!is_stopped_.compare_exchange_strong(expected, true)) return; 
        
        stop_flag_.store(true, std::memory_order_release);

        if (consumer_done_future_.valid()) {
            consumer_done_future_.wait();
        }
    }

private:
    SPSCQueue<T, Capacity> queue_;
    std::atomic<bool> stop_flag_;
    std::atomic<bool> is_stopped_;
    std::future<void> consumer_done_future_;
};