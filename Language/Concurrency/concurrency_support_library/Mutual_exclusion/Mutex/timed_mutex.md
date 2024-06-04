# timed_mutex

参考文档

* [timed_mutex](https://en.cppreference.com/w/cpp/thread/timed_mutex)

## 类原型

```CPP
class timed_mutex;
```

定义在头文件`<mutex>`中。

## 描述

`timed_mutex`类是一个同步原语，可以被用在保护共享资源防止竞争。

和`mutex`相似，`timed_mutex`类提供一个独占的非递归的所有权语义。此外，它为函数`try_lock_for()`,`try_lock_until()`还提供了超时机制.

`timed_mutex`必须满足`TimedMutex`与`StandardLayoutType`的要求。

## 成员类定义

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## 成员函数

### 构造与析构

* [timed_mutex](https://en.cppreference.com/w/cpp/thread/timed_mutex/timed_mutex)

  ```CPP
  timed_mutex();
  timed_mutex( const timed_mutex& ) = delete;
  ```

  构造`timed_mutex`类，构造完毕后，`timed_mutex`类处于`unlock`状态。

### 锁定与解锁

* [lock](https://en.cppreference.com/w/cpp/thread/mutex/lock)

  ```CPP
  void lock();
  ```

  锁定`timed_mutex`，如果其它的线程已经锁定了这个`timed_mutex`，这个调用会使得这个线程被阻塞直到可以获得这个`timed_mutex`的所有权为止。

  如果这个线程已经有了这个`timed_mutex`的所有权，程序行为未定义。

  在同一个`timed_mutex`上之前的`unlock`操作与`lock`同步，(std::memory_order中的定义).

  `lock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。

* [try_lock](https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试锁定`timed_mutex`,立即返回，如果成功获得`timed_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果这个线程已经有了这个`timed_mutex`的所有权，程序行为未定义。

  如果返回`true` 在同一个`timed_mutex`上之前的`unlock`操作与`try_lock`同步.

* [try_lock_for](https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_for)

  ```CPP
  template< class Rep, class Period >
  bool try_lock_for( const std::chrono::duration<Rep, Period>& timeout_duration );
  ```

  尝试锁定`timed_mutex`,阻塞直到指定的`timeout_duration`时间过去或者是成功锁定`timed_mutex`.如果成功获得`timed_mutex`的所有权就返回`true`,否则返回`false`.

  由于调度器延迟等，线程可能会阻塞超过`timeout_duration`。

  如果`timeout_duration`小于或等于`timeout_duration.zero()`,函数行为如同`try_lock()`.

  哪怕是`timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果这个线程已经有了这个`timed_mutex`的所有权，程序行为未定义。

  如果返回`true` 在同一个`timed_mutex`上之前的`unlock`操作与`try_lock`同步.

* [try_lock_until](https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_until)

  ```CPP
  template< class Clock, class Duration >
  bool try_lock_until( const std::chrono::time_point<Clock, Duration>& timeout_time );
  ```

  尝试锁定`timed_mutex`,阻塞直到指定的`timeout_time`到达或者是成功锁定`timed_mutex`.如果成功获得`timed_mutex`的所有权就返回`true`,否则返回`false`.

  如果`timeout_time`早已到达，那么函数行为如同`try_lock`.

  由于调度器延迟等，线程可能会阻塞超过指定时间。

  哪怕是`timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果这个线程已经有了这个`timed_mutex`的所有权，程序行为未定义。

  如果返回`true` 在同一个`timed_mutex`上之前的`unlock`操作与`try_lock`同步.

* [unlock](https://en.cppreference.com/w/cpp/thread/timed_mutex/unlock)

  ```CPP
  void unlock();
  ```

  解锁`timed_mutex`.

  当前线程必须已经锁定了这个`timed_mutex`,否则程序行为未定义。

  这个操作与接下来的`lock`相同`timed_mutex`的序列同步。

  `unlock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。
