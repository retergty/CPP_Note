#include <vector>
#include <memory>
#include <thread>
#include <iostream>
#include <functional>
#include "BlockMPMC.hpp"
// 假设这是我们的任务结构体
struct Task {
    int target_consumer_id; // 决定这个任务归谁管
    std::string payload;
};

class DataRouter {
public:
    // 初始化时，直接创建 N 个队列和 N 个消费者线程
    explicit DataRouter(size_t consumer_count) : consumer_count_(consumer_count) {
        for (size_t i = 0; i < consumer_count_; ++i) {
            // 给每个消费者创建一个专属队列
            queues_.emplace_back(std::make_unique<BlockingRingBuffer<Task>>(1024));
            
            // 启动消费者线程，只监听自己的那个专属队列
            workers_.emplace_back(&DataRouter::worker_loop, this, i);
        }
    }

    ~DataRouter() {
        for (auto& q : queues_) { q->close(); }
        for (auto& w : workers_) { if (w.joinable()) w.join(); }
    }

    // 生产者的分发接口
    void dispatch(Task task) {
        // 【核心路由逻辑】：通过取模运算，把任务精确投递到专属信箱
        // 如果 target_consumer_id 是用户 ID，这里就是一致性 Hash 的雏形！
        size_t target_queue_idx = task.target_consumer_id % consumer_count_;
        
        // 推入对应消费者的专属队列（此时是 MPSC 模型：多生产者，单消费者）
        queues_[target_queue_idx]->push(std::move(task));
    }

private:
    void worker_loop(size_t my_id) {
        Task current_task;
        // 消费者极其幸福，不需要抢锁，专属队列里出来的绝对都是自己的活儿
        while (queues_[my_id]->pop(current_task)) {
            // std::cout << "Worker " << my_id << " processing task for " 
            //           << current_task.target_consumer_id << "\n";
        }
    }

    size_t consumer_count_;
    // N 个独立的队列（信箱）
    std::vector<std::unique_ptr<BlockingRingBuffer<Task>>> queues_; 
    // N 个消费者线程
    std::vector<std::thread> workers_; 
};
