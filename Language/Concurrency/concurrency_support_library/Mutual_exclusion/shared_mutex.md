# shared_mutex

参考文档

* [shared_mutex](https://en.cppreference.com/w/cpp/thread/shared_mutex)

## 类原型

```CPP
class shared_mutex;
```

定义在头文件`<shared_mutex>`中。

## 描述

`shared_mutex`类是一个同步原语，可以被用在保护共享资源防止竞争。

不同于其它的互斥锁类型，`shared_mutex`有两个访问等级：

* 共享(shared),几个线程可以共享同一个互斥锁的所有权
* 独占(exclusive),只有一个线程可以拥有这个互斥锁。

如果一个线程获得了独占级别的访问（调用`lock`,`try_lock`），别的任何线程都不能获得这个锁（包括共享级别）。

如果一个线程获得了共享级别的访问(调用`lock_shared`,`try_lock_shared`),别的任何线程不能获得独占级别的锁，但是可以获得共享级别的锁。

在一个线程里，同一个`shared_mutex`在同一时间只能有处于一个级别（共享或者独占）。

`shared_mutex`在大量线程同时读取时，少量线程写入时很有用。

`shared_mutex`类不是可复制和可移动的。

## 成员类定义

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## 成员函数

### 构造与析构

* [shared_mutex](https://en.cppreference.com/w/cpp/thread/shared_mutex/shared_mutex)

  ```CPP
  shared_mutex();
  shared_mutex( const shared_mutex& ) = delete;
  ```

  构造`shared_mutex`类，构造完毕后，`shared_mutex`类处于`unlock`状态。

### 独占级别

* [lock](https://en.cppreference.com/w/cpp/thread/shared_mutex/lock)

  ```CPP
  void lock();
  ```

  获取独占等级的`shared_mutex`,如果别的线程已经拥有独占级别或共享级别的`shared_mutex`，阻塞当前进程直到可以获得这个`shared_mutex`的所有权为止。

  如果调用线程早已获得了`shared_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock](https://en.cppreference.com/w/cpp/thread/shared_mutex/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试独占锁定`shared_mutex`,立即返回，如果成功获得`shared_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`shared_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_mutex`(不论什么访问级别)，代码行为未定义。

* [unlock](https://en.cppreference.com/w/cpp/thread/shared_mutex/unlock)

  ```CPP
  void unlock();
  ```

  解锁`shared_mutex`的独占访问。

  当前线程必须已经获得了这个`shared_mutex`的独占所有权,否则程序行为未定义。

### 共享级别

* [lock_shared](https://en.cppreference.com/w/cpp/thread/shared_mutex/lock_shared)

  ```CPP
  void lock_shared();
  ```

  获取共享等级的`shared_mutex`,如果别的线程已经拥有独占级别级别的`shared_mutex`，阻塞当前进程直到可以获得这个`shared_mutex`的所有权为止。

  如果调用线程早已获得了`shared_mutex`(不论什么访问级别)，代码行为未定义。

  如果有超过某个最大数量的线程都在获得`shared_mutex`共享级别，这个线程也会阻塞，最大值取决于实现。

* [try_lock_shared](https://en.cppreference.com/w/cpp/thread/shared_mutex/try_lock_shared)

  ```CPP
  bool try_lock_shared();
  ```

  尝试获取共享等级的`shared_mutex`,立即返回，如果成功获得`shared_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`shared_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_mutex`(不论什么访问级别)，代码行为未定义。

* [unlock_shared](https://en.cppreference.com/w/cpp/thread/shared_mutex/unlock_shared)

  ```CPP
  void unlock_shared();
  ```

  解锁`shared_mutex`的共享访问。

  当前线程必须已经获得了这个`shared_mutex`的共享所有权,否则程序行为未定义。

## 例子

```CPP
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
 
class ThreadSafeCounter
{
public:
    ThreadSafeCounter() = default;
 
    // Multiple threads/readers can read the counter's value at the same time.
    unsigned int get() const
    {
        std::shared_lock lock(mutex_);
        return value_;
    }
 
    // Only one thread/writer can increment/write the counter's value.
    void increment()
    {
        std::unique_lock lock(mutex_);
        ++value_;
    }
 
    // Only one thread/writer can reset/write the counter's value.
    void reset()
    {
        std::unique_lock lock(mutex_);
        value_ = 0;
    }
 
private:
    mutable std::shared_mutex mutex_;
    unsigned int value_{};
};
 
int main()
{
    ThreadSafeCounter counter;
 
    auto increment_and_print = [&counter]()
    {
        for (int i{}; i != 3; ++i)
        {
            counter.increment();
            std::osyncstream(std::cout)
                << std::this_thread::get_id() << ' ' << counter.get() << '\n';
        }
    };
 
    std::thread thread1(increment_and_print);
    std::thread thread2(increment_and_print);
 
    thread1.join();
    thread2.join();
}
```
