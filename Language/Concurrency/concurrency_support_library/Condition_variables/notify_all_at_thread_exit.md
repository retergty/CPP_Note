# notify_all_at_thread_exit

参考文档

* [notify_all_at_thread_exit](https://en.cppreference.com/w/cpp/thread/notify_all_at_thread_exit)

定义在头文件`<condition_variable>`中.

## 函数原型

```CPP
void notify_all_at_thread_exit( std::condition_variable& cond,
                                std::unique_lock<std::mutex> lk );
```

## 描述

`notify_all_at_thread_exit`提供了一个当前线程完全执行完毕后才通知其它线程的方法。

* 先前获取的锁lk的所有权被转移到内部存储
* 修改处理环境，使得当线程退出时，条件变量可以通知，就好像执行了`lk.unlock();cond.notify_all();`.

`lk.unlock()`发生在当前线程所有本地变量都被析构后

如果以下条件满足，则行为未定义

* `lk`没有被当前线程锁定
* 等待这个条件变量的线程没有使用与`lk`相同的互斥锁。

也可以使用`std::promise`,`std::packaged_task`达到相同的效果。

会一直锁定`lk`的互斥锁直到线程完全执行完毕，当这个函数调用时，其余线程不可能获取这个互斥锁从而等待条件变量。如果有线程正在等待这个条件变量，程序需要在锁定`lk`时，确保等待的条件满足，且在调用`notify_all_at_thread_exit`前，不应该再度释放并获取互斥锁，避免虚假唤起。

## 例子

该部分代码片段说明了使用`notify_all_at_thread_exit`来避免访问依赖于线程局部变量的数据，而这些线程局部变量正在被破坏的过程中：

```CPP
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
 
std::mutex m;
std::condition_variable cv;
 
bool ready = false;
std::string result; // some arbitrary type
 
void thread_func()
{
    thread_local std::string thread_local_data = "42";
 
    std::unique_lock<std::mutex> lk(m);
 
    // assign a value to result using thread_local data
    result = thread_local_data;
    ready = true;
 
    std::notify_all_at_thread_exit(cv, std::move(lk));
 
}   // 1. destroy thread_locals;
    // 2. unlock mutex;
    // 3. notify cv.
 
int main()
{
    std::thread t(thread_func);
    t.detach();
 
    // do other work
    // ...
 
    // wait for the detached thread
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, []{ return ready; });
 
    // result is ready and thread_local destructors have finished, no UB
    assert(result == "42");
}
```
