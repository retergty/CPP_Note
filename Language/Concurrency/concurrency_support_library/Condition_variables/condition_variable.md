# condition_variable

条件变量(condition variable)提供了多个线程间传递信息的方法。

参考文档

* [condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable)

定义在头文件`<condition_variable>`中.

## 类定义

```CPP
class condition_variable;
```

## 描述

条件变量是一个同步原语，它通常和`std::mutex`一起使用，阻塞一个或者多个线程直到别的线程修改了条件变量并通知给`condition_variable`.

想要修改条件变量的线程必须：

* 获取到对应的`std::mutex`所有权（通常是通过`std::lock_guard`）.
* 修改与条件变量相关的变量的值。
* 调用条件变量的成员函数`notify_one`或者是`notify_all`。（可以在释放了互斥锁之后做）。

哪怕是条件变量是原子的，为了可以正确地通知别的线程，也必须使用互斥锁来修改条件变量。

想要等待条件变量的线程必须：

* 获取对应的`std::mutex`的所有权，也就是写入这个条件变量的互斥锁，通过`std::unique_lock<std::mutex>`
* 检查条件变量的值是否已经更新并通知了。
* 调用`wait`，`wait_for`,`wair_until`成员函数（原子地释放互斥锁，并挂起线程直到满足要求或者超时，并原子地获取互斥锁）
* 检查条件变量的值，如果没有更新，就继续等待。

或者是：

* 获取对应的`std::mutex`的所有权，也就是写入这个条件变量的互斥锁，通过`std::unique_lock<std::mutex>`
* 使用谓词版本的`wait`，`wait_for`,`wair_until`成员函数（它会把上面三步合一）.

`std::condition_variable`只能与`std::unique_lock<std::mutex>`一起使用,`std::condition_variable_any`可以与任何互斥锁一起使用。

`std::condition_variable`允许多线程同时调用`wait`,`wait_for`,`wait_until`成员函数。

`std::condition_variable`不是可复制与可移动的。

## 成员类定义

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## 成员函数

### 构造与析构

* [condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable/condition_variable)

* [~condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable/%7Econdition_variable)

  ```CPP
  ~condition_variable();
  ```

  只有在所有的线程都被通知后才能调用这个析构函数，这不意味着线程需要完全退出对应的等待函数，它们可能在等待重新获取互斥锁，或者在获取互斥锁后被等待调度器的切换。

  程序必须保证当析构函数开始时，没有线程尝试去等待`*this`。

### 通知线程

* [notify_one](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one)

  ```CPP
  void notify_one() noexcept;
  ```

  如果有任意个线程正在等待`*this`,唤醒其中一个线程。

* [notify_all](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_all)

  ```CPP
  void notify_all() noexcept;
  ```

  唤醒所有正在等待`*this`的线程。

`notify_one`和`notify_all`与`wait()`,`wait_for()`,`wait_until()`的三个原子部分在单个顺序中顺序发生，叫做`modification order`.也就是说，`notify_one/notify_all`不可能唤醒那些在`notify_one/notify_all`后才等待的线程。

只是调用通知函数的话，这个线程不需要获取关联的互斥锁。因为被通知的线程在唤醒后又会由于获取这个互斥锁而进入阻塞。一些具体实现可能会更加聪明，直接把被唤醒的线程从一个等待队列中移到另一个队列。

然而，当需要精确安排事件时，在锁定期间进行通知可能是必要的。比如如果等待线程在条件变量满足后就会退出，引发条件变量的析构，这样其它正在等待这个条件变量的线程就会出错。

### 等待条件变量

* [wait](https://en.cppreference.com/w/cpp/thread/condition_variable/wait)

  ```CPP
  void wait( std::unique_lock<std::mutex>& lock );
  template< class Predicate >
  void wait( std::unique_lock<std::mutex>& lock, Predicate pred );
  ```

  阻塞当前进程直到条件变量被通知或者是一个虚假的唤起发生。`pred`可以用于检测虚假唤起。

  `1`首先原子性地调用`lock.unlock()`并阻塞当前进程直到`notify_all()/notify_one()`被别的线程调用，或者出现虚假的唤起，之后在原子性地调用`lock.lock()`并返回。

  `2`等价于

  ```CPP
  while (!pred())
    wait(lock);
  ``

  `pred()`不需要是原子的，因为此时互斥锁还保持锁定。

  所有等待同一个条件变量地线程都要使用同一个`mutex`.

* [wait_for](https://en.cppreference.com/w/cpp/thread/condition_variable/wait_for)

  ```CPP
  template< class Rep, class Period >
  std::cv_status wait_for( std::unique_lock<std::mutex>& lock,
                          const std::chrono::duration<Rep, Period>& rel_time );
  template< class Rep, class Period, class Predicate >
  bool wait_for( std::unique_lock<std::mutex>& lock,
                const std::chrono::duration<Rep, Period>& rel_time,
                Predicate pred );
  ```

  阻塞当前进程直到条件变量被通知或者是一个虚假的唤起发生或者是超时。`pred`可以用于检测虚假唤起。

* [wait_until](https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until)

  ```CPP
  template< class Clock, class Duration >
  std::cv_status
      wait_until( std::unique_lock<std::mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& abs_time );
  template< class Clock, class Duration, class Predicate >
  bool wait_until( std::unique_lock<std::mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& abs_time,
                  Predicate pred );
  ```

  阻塞当前进程直到条件变量被通知或者是一个虚假的唤起发生或者是超时。`pred`可以用于检测虚假唤起。

## 虚假唤起

参考文档

* [Spurious wakeup](https://en.wikipedia.org/wiki/Spurious_wakeup)

虚假唤起(spurious wakeup)发生在一个等待线程被唤醒，但是条件变量还没有满足。好像是这个线程毫无理由被唤醒。但是，它经常是发生在当条件变量被改变与等待线程最后运行这段时间内（也就是说互斥锁的空隙），其它的线程又改变了条件变量，引起了竞争。所以当等待线程运行时，虚假唤起便发生了。

在一些多核操作系统上，虚假唤起加剧，如果有多个线程等待同一个条件变量，系统可能会认为唤起一个线程的系统调用`signal( )`为广播`broadcast( )`唤起所有的线程，这样带来了虚假唤起。

对于不同的实现，标准也允许等待线程哪怕是没有被通知也会唤起，一些操作系统可能也会唤起这个线程。(Linux保证不会发生)。

因为可能会发生竞争，最好在等待线程唤起时，检测唤醒条件是否满足（此时是互斥锁锁定状态，不必担心竞争），如果没有满足，返回到等待状态。

## 实用类定义

### cv_status

```CPP
enum class cv_status {
    no_timeout,
    timeout  
};
```

描述了`wait_for`,`wait_until`是因为什么而退出的。

## 例子

```CPP
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
 
std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;
 
void worker_thread()
{
    // wait until main() sends data
    std::unique_lock lk(m);
    cv.wait(lk, []{ return ready; });
 
    // after the wait, we own the lock
    std::cout << "Worker thread is processing data\n";
    data += " after processing";
 
    // send data back to main()
    processed = true;
    std::cout << "Worker thread signals data processing completed\n";
 
    // manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lk.unlock();
    cv.notify_one();
}
 
int main()
{
    std::thread worker(worker_thread);
 
    data = "Example data";
    // send data to the worker thread
    {
        std::lock_guard lk(m);
        ready = true;
        std::cout << "main() signals data ready for processing\n";
    }
    cv.notify_one();
 
    // wait for the worker
    {
        std::unique_lock lk(m);
        cv.wait(lk, []{ return processed; });
    }
    std::cout << "Back in main(), data = " << data << '\n';
 
    worker.join();
}
```

* 依照调度器的实现，可能主线程快速设置了`ready=true`导致工作线程不用等待，快速到达下面，所以使用了`ready`与`processed`.不能只使用一个变量，如果主线程快速运行到`cv.wait`，那么使用同一个变量就会导致错误的等待退出。
