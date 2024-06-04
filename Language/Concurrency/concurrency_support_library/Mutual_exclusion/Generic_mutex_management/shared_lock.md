# shared_lock

参考文档

* [shared_lock](https://en.cppreference.com/w/cpp/thread/shared_lock)

## 类原型

```CPP
template< class Mutex >
class shared_lock;
```

定义在头文件`<shared_mutex>`中。

`Mutex`是锁定的`mutex`的类型，必须符合`SharedLockable`要求。

## 描述

`shared_lock`是一个通用的互斥锁包装器，允许延迟锁定，具有超时机制的锁定，传递所有权.用于在共享等级下锁定一个`shared_mutex`(为了在独占等级下锁定一个`shared_mutex`，可以使用`unique_lock`).

`shared_lock`同时还会记录锁定`mutex`的线程，从而可以检测是否`mutex`已经锁定。

`shared_lock`是可移动但不可复制的，满足复制可构造和复制可赋值，但不满足移动可构造和移动可赋值。

## 成员类定义

* `mutex_type`就是`Mutex`

## 成员函数

### 构造与析构

* [shared_lock](https://en.cppreference.com/w/cpp/thread/shared_lock/shared_lock)

  ```CPP
  shared_lock() noexcept;
  shared_lock( shared_lock&& other ) noexcept;
  explicit shared_lock( mutex_type& m );
  shared_lock( mutex_type& m, std::defer_lock_t t ) noexcept;
  shared_lock( mutex_type& m, std::try_to_lock_t t );
  shared_lock( mutex_type& m, std::adopt_lock_t t );
  template< class Rep, class Period >
  shared_lock( mutex_type& m,
              const std::chrono::duration<Rep,Period>& timeout_duration );
  template< class Clock, class Duration >
  shared_lock( mutex_type& m,
              const std::chrono::time_point<Clock,Duration>& timeout_time );
  ```

  构造`shared_lock`,

  `1`默认构造函数

  `2`移动构造函数，初始化`shared_lock`，将`other`的`mutex`所有权转交给`*this`.

  `3-8`构造`shared_lock`，并让它管理`m`的所有权。根据剩下的参数的不同，决定使用的锁定函数，`defer_lock_t`表示目前先不调用`lock`。如果使用时间，则互斥锁也必须是`timed_mutex`.

* [~shared_lock](https://en.cppreference.com/w/cpp/thread/shared_lock/~shared_lock)

  ```CPP
  ~shared_lock();
  ```

  析构`shared_lock`.

  如果`*this`有与之相关联的`mutex`，（`mutex()`函数返回非空），且已经锁定这个`mutex`，（`own()`返回`true`），则调用`mutex()->unlock_shared()`释放所有权。

* [operator=](https://en.cppreference.com/w/cpp/thread/shared_lock/operator%3D)

  ```CPP
  shared_lock& operator=( shared_lock&& other ) noexcept;
  ```

  移动构造函数。

## 共享锁定

* [lock](https://en.cppreference.com/w/cpp/thread/shared_lock/lock)

  ```CPP
  void lock();
  ```

  共享级别锁定`mutex`,等价于调用`mutex()->lock_shared()`.

* [try_lock](https://en.cppreference.com/w/cpp/thread/shared_lock/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试共享级别锁定`mutex`,等价于调用`mutex()->try_lock_shared()`.

* [try_lock_for](https://en.cppreference.com/w/cpp/thread/shared_lock/try_lock_for)

  ```CPP
  template< class Rep, class Period >
  bool try_lock_for( const std::chrono::duration<Rep,Period>& timeout_duration );
  ```

  尝试具有超时机制的共享级别锁定`mutex`，等价于调用`mutex()->try_lock_shared_for(timeout_duration)`.

* [try_lock_until](https://en.cppreference.com/w/cpp/thread/shared_lock/try_lock_until)

  ```CPP
  template< class Clock, class Duration >
  bool try_lock_until( const std::chrono::time_point<Clock,Duration>& timeout_time );
  ```

  尝试具有超时机制的共享级别锁定`mutex`，等价于调用`mutex()->try_lock_shared_until(timeout_time)`.

* [unlock](https://en.cppreference.com/w/cpp/thread/shared_lock/unlock)

  ```CPP
  void unlock();
  ```

  释放`mutex`的所有权，等价于调用`mutex()->unlock_shared()`.

## 修改类本身

* [swap](https://en.cppreference.com/w/cpp/thread/shared_lock/swap)

  ```CPP
  template< class Mutex >
  void swap( shared_lock<Mutex>& other ) noexcept;
  ```

  交换两个`shared_lock`的内部状态。

* [release](https://en.cppreference.com/w/cpp/thread/shared_lock/release)

  ```CPP
  mutex_type* release() noexcept;
  ```

  取消目前与`shared_lock`所关联的`mutex`,但是不调用`unlock`.

  返回指向这个`mutex`的指针，或者是`nullptr`（如果没有与`shared_lock`所关联的`mutex`).

## 获取信息

* [mutex](https://en.cppreference.com/w/cpp/thread/shared_lock/mutex)

  ```CPP
  mutex_type* mutex() const noexcept;
  ```

  返回`*this`所关联的`mutex`,如果没有，则是`nullptr`.

* [owns_lock](https://en.cppreference.com/w/cpp/thread/shared_lock/owns_lock)

  ```CPP
  bool owns_lock() const noexcept;
  ```

  检查是否`*this`管理着`mutex`且这个`mutex`已经锁定了。

* [operator bool](https://en.cppreference.com/w/cpp/thread/shared_lock/operator_bool)

  ```CPP
  explicit operator bool() const noexcept;
  ```

  检查是否`*this`管理着`mutex`且这个`mutex`已经锁定了。内部调用`owns_lock`.
