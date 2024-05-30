# unique_lock

参考文档

* [unique_lock](https://en.cppreference.com/w/cpp/thread/unique_lock)

## 类原型

```CPP
template< class Mutex >
class unique_lock;
```

定义在头文件`<mutex>`中。

`Mutex`是锁定的`mutex`的类型，必须符合`BasicLockable`要求。

## 描述

`unique_lock`是一个通用的互斥锁包装器，允许延迟锁定，时间限制的锁定，递归锁定，转移互斥锁的所有权以及与条件变量结合使用。

`unique_lock`同时还会记录锁定`mutex`的线程，从而可以检测是否`mutex`已经锁定。

`unique_lock`可以移动但是不能复制，满足复制可构造和复制可赋值，但不满足移动可构造和移动可赋值。

## 成员类定义

* `mutex_type`就是`Mutex`

## 成员函数

### 构造与析构

* [unique_lock](https://en.cppreference.com/w/cpp/thread/unique_lock/unique_lock)

  ```CPP
  unique_lock() noexcept;
  unique_lock( unique_lock&& other ) noexcept;
  explicit unique_lock( mutex_type& m );
  unique_lock( mutex_type& m, std::defer_lock_t t ) noexcept;
  unique_lock( mutex_type& m, std::try_to_lock_t t );
  unique_lock( mutex_type& m, std::adopt_lock_t t );
  template< class Rep, class Period >
  unique_lock( mutex_type& m,
              const std::chrono::duration<Rep, Period>& timeout_duration );
  template< class Clock, class Duration >
  unique_lock( mutex_type& m,
              const std::chrono::time_point<Clock, Duration>& timeout_time );
  ```

  构造`unique_lock`,

  `1`默认构造函数

  `2`移动构造函数，初始化`unique_lock`，将`other`的`mutex`所有权转交给`*this`.

  `3-8`构造`unique_lock`，并让它管理`m`的所有权。根据剩下的参数的不同，决定使用的锁定函数，`defer_lock_t`表示目前先不调用`lock`。如果使用时间，则互斥锁也必须是`timed_mutex`.

* [~unique_lock](https://en.cppreference.com/w/cpp/thread/unique_lock/~unique_lock)

  ```CPP
  ~unique_lock();
  ```

  销毁`unique_lock`,如果`*this`正在管理一个互斥锁的所有权，解锁`mutex`.

* [operator=](https://en.cppreference.com/w/cpp/thread/unique_lock/operator%3D)

  ```CPP
  unique_lock& operator=( unique_lock&& other );
  ```

  移动赋值函数，转移`other`所管理的`mutex`的所有权，如果`*this`目前已经锁定了一个`mutex`，解锁这个`mutex`.

### 锁定与解锁

* [lock](https://en.cppreference.com/w/cpp/thread/unique_lock/lock)

  ```CPP
  void lock();
  ```

  锁定`unique_lock`所关联的`mutex`，等价于调用`mutex()->lock()`.

* [try_lock](https://en.cppreference.com/w/cpp/thread/unique_lock/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试锁定`unique_lock`所关联的`mutex`，等价于调用`mutex()->try_lock()`.

* [try_lock_for](https://en.cppreference.com/w/cpp/thread/unique_lock/try_lock_for)

  ```CPP
  template< class Rep, class Period >
  bool try_lock_for( const std::chrono::duration<Rep, Period>& timeout_duration );
  ```

  尝试锁定`mutex`，具有超时机制。等价于调用`mutex()->try_lock_for(timeout_duration)`.

* [try_lock_until](https://en.cppreference.com/w/cpp/thread/unique_lock/try_lock_until)

  ```CPP
  template< class Clock, class Duration >
  bool try_lock_until( const std::chrono::time_point<Clock, Duration>& timeout_time );
  ```

  尝试锁定`mutex`，具有超时机制。等价于调用`mutex()->try_lock_until(timeout_time)`.

* [unlock](https://en.cppreference.com/w/cpp/thread/unique_lock/unlock)

  ```CPP
  void unlock();
  ```

  解锁`mutex`,等价于调用`mutex()->unlock()`.如果没有与`unique_lock`相关联的`mutex`或没有锁定，则抛出异常。

### 修改类

* [swap](https://en.cppreference.com/w/cpp/thread/unique_lock/swap)

  ```CPP
  void swap( unique_lock& other ) noexcept;
  ```

  交换两个`unique_lock`的状态.

* [release](https://en.cppreference.com/w/cpp/thread/unique_lock/release)

  取消目前与`unique_lock`所关联的`mutex`,但是不调用`unlock`.

### 获取信息

* [mutex](https://en.cppreference.com/w/cpp/thread/unique_lock/mutex)

  ```CPP
  mutex_type* mutex() const noexcept;
  ```

  返回`*this`所关联的`mutex`,如果没有，则是`nullptr`.

* [owns_lock](https://en.cppreference.com/w/cpp/thread/unique_lock/owns_lock)

  ```CPP
  bool owns_lock() const noexcept;
  ```

  检查是否`*this`管理着`mutex`且这个`mutex`已经锁定了。

* [operator bool](https://en.cppreference.com/w/cpp/thread/unique_lock/operator_bool)

  ```CPP
  explicit operator bool() const noexcept;
  ```

  检查是否`*this`管理着`mutex`且这个`mutex`已经锁定了。内部调用`owns_lock`.

## 例子

```CPP
#include <iostream>
#include <mutex>
#include <thread>
 
struct Box
{
    explicit Box(int num) : num_things{num} {}
 
    int num_things;
    std::mutex m;
};
 
void transfer(Box& from, Box& to, int num)
{
    // don't actually take the locks yet
    std::unique_lock lock1{from.m, std::defer_lock};
    std::unique_lock lock2{to.m, std::defer_lock};
 
    // lock both unique_locks without deadlock
    std::lock(lock1, lock2);
 
    from.num_things -= num;
    to.num_things += num;
 
    // “from.m” and “to.m” mutexes unlocked in unique_lock dtors
}
 
int main()
{
    Box acc1{100};
    Box acc2{50};
 
    std::thread t1{transfer, std::ref(acc1), std::ref(acc2), 10};
    std::thread t2{transfer, std::ref(acc2), std::ref(acc1), 5};
 
    t1.join();
    t2.join();
 
    std::cout << "acc1: " << acc1.num_things << "\n"
                 "acc2: " << acc2.num_things << '\n';
}
```