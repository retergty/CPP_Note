#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>

class FixedThreadPool {
public:
    // 构造函数：启动固定数量的 Worker 线程
    explicit FixedThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;

                    {
                        // 1. 加锁，准备从队列取任务
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        
                        // 2. 等待条件：要么线程池停止了，要么队列里有任务了
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->tasks.empty(); 
                        });

                        // 3. 如果收到停止信号且队列为空，线程安全退出
                        if(this->stop && this->tasks.empty()) {
                            return;
                        }

                        // 4. 取出任务
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    } // 离开作用域，自动解锁

                    // 5. 执行任务（在锁外执行，绝对不能阻塞其他线程取任务）
                    task();
                }
            });
        }
    }

    // 提交任务到线程池，返回 std::future 以获取结果
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;

        // 将任务和参数打包成一个 shared_ptr 的 packaged_task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            // 把任务封装成无参数、无返回值的 void() 函数放入队列
            tasks.emplace([task]() { (*task)(); });
        }
        
        // 唤醒一个正在睡眠的 Worker 线程
        condition.notify_one();
        return res;
    }

    // 析构函数：优雅地关闭线程池
    ~FixedThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        // 唤醒所有线程，让它们检查 stop 标志并退出
        condition.notify_all();
        for(std::thread &worker : workers) {
            if(worker.joinable()) {
                worker.join(); // 等待所有线程执行完剩余任务后安全销毁
            }
        }
    }

private:
    std::vector<std::thread> workers;          // 存放真实操作系统的线程
    std::queue<std::function<void()>> tasks;   // 任务队列

    std::mutex queue_mutex;                    // 保护队列的互斥锁
    std::condition_variable condition;         // 用于阻塞和唤醒线程的条件变量
    bool stop;                                 // 线程池停止标志
};
