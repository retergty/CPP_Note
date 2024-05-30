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