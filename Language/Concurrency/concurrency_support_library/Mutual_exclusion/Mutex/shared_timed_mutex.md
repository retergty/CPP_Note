# shared_timed_mutex

参考文档

* [shared_timed_mutex](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex)

## 类原型

```CPP
class shared_timed_mutex;
```

定义在头文件`<shared_mutex>`中。

## 描述

`shared_timed_mutex`类是一个同步原语，可以被用在保护共享资源防止竞争。

不同于其它的互斥锁类型，`shared_timed_mutex`有两个访问等级：

* 共享(shared),几个线程可以共享同一个互斥锁的所有权
* 独占(exclusive),只有一个线程可以拥有这个互斥锁。

`shared_timed_mutex`可以用来实现读写锁。

`shared_mutex`类不是可复制和可移动的。

## 成员函数

### 构造与析构

* [shared_timed_mutex](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/shared_timed_mutex)

  ```CPP
  shared_timed_mutex();
  shared_timed_mutex( const shared_timed_mutex& ) = delete;
  ```

  构造`shared_timed_mutex`类，构造完毕后，`shared_timed_mutex`类处于`unlock`状态。

### 独占级别

* [lock](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/lock)

  ```CPP
  void lock();
  ```

  获取独占等级的`shared_timed_mutex`,如果别的线程已经拥有独占级别或共享级别的`shared_timed_mutex`，阻塞当前进程直到可以获得这个`shared_timed_mutex`的所有权为止。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试独占锁定`shared_timed_mutex`,立即返回，如果成功获得`shared_timed_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock_for](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock_for)

  ```CPP
  template< class Rep, class Period >
  bool try_lock_for( const std::chrono::duration<Rep, Period>& timeout_duration );
  ```

  尝试独占锁定`shared_timed_mutex`,如果别的线程已经拥有独占级别或共享级别的`shared_timed_mutex`，则阻塞线程，直到超出等待时长`timeout_duration`后返回`false`或成功锁定`shared_timed_mutex`返回`true`.

  由于调度器延迟等，线程可能会阻塞超过`timeout_duration`。

  如果`timeout_duration`小于或等于`timeout_duration.zero()`,函数行为如同`try_lock()`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock_until](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock_until)

  ```CPP
  template< class Clock, class Duration >
  bool try_lock_until( const std::chrono::time_point<Clock, Duration>& timeout_time );
  ```

  尝试独占锁定`shared_timed_mutex`,如果别的线程已经拥有独占级别或共享级别的`shared_timed_mutex`，则阻塞线程，直到超出等待时间`timeout_time`后返回`false`或成功锁定`shared_timed_mutex`返回`true`.

  如果`timeout_time`早已到达，那么函数行为如同`try_lock`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [unlock](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/unlock)

  ```CPP
  void unlock();
  ```

  解锁`shared_timed_mutex`的独占访问。

  当前线程必须已经获得了这个`shared_timed_mutex`的独占所有权,否则程序行为未定义。

### 共享级别

* [lock_shared](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/lock_shared)

  ```CPP
  void lock_shared();
  ```

  获取共享等级的`shared_timed_mutex`,如果别的线程已经拥有独占级别级别的`shared_timed_mutex`，阻塞当前进程直到可以获得这个`shared_timed_mutex`的所有权为止。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

  如果有超过某个最大数量的线程都在获得`shared_timed_mutex`共享级别，这个线程也会阻塞，最大值取决于实现。

* [try_lock_shared](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock_shared)

  ```CPP
  bool try_lock_shared();
  ```

  尝试获取共享等级的`shared_timed_mutex`,立即返回，如果成功获得`shared_timed_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock_shared_for](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock_shared_for)

  ```CPP
  template< class Rep, class Period >
  bool try_lock_shared_for( const std::chrono::duration<Rep,Period>& timeout_duration );
  ```

  尝试获取`shared_timed_mutex`的共享所有权，如果别的线程已经拥有独占级别的`shared_timed_mutex`，则阻塞线程，直到超出等待时长`timeout_duration`后返回`false`或成功锁定`shared_timed_mutex`返回`true`.

  由于调度器延迟等，线程可能会阻塞超过`timeout_duration`。

  如果`timeout_duration`小于或等于`timeout_duration.zero()`,函数行为如同`try_lock()`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [try_lock_shared_until](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/try_lock_shared_until)

  ```CPP
  template< class Clock, class Duration >
  bool try_lock_shared_until( const std::chrono::time_point<Clock,Duration>& timeout_time );
  ```

  尝试获取`shared_timed_mutex`的共享所有权，如果别的线程已经拥有独占级别的`shared_timed_mutex`，则阻塞线程，直到超出等待时间`timeout_time`后返回`false`或成功锁定`shared_timed_mutex`返回`true`.

  如果`timeout_time`早已到达，那么函数行为如同`try_lock`.

  哪怕是`shared_timed_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果调用线程早已获得了`shared_timed_mutex`(不论什么访问级别)，代码行为未定义。

* [unlock_shared](https://en.cppreference.com/w/cpp/thread/shared_timed_mutex/unlock_shared)

  ```CPP
  void unlock_shared();
  ```

  解锁`shared_mutex`的共享访问。

  当前线程必须已经获得了这个`shared_mutex`的共享所有权,否则程序行为未定义。

## 例子

一个复制构造函数可以处理多个读取，但是只能处理一个写入

```CPP
#include <mutex>
#include <shared_mutex>
 
class R
{
    mutable std::shared_timed_mutex mut;
    /* data */
public:
    R& operator=(const R& other)
    {
        // requires exclusive ownership to write to *this
        std::unique_lock<std::shared_timed_mutex> lhs(mut, std::defer_lock);
        // requires shared ownership to read from other
        std::shared_lock<std::shared_timed_mutex> rhs(other.mut, std::defer_lock);
        std::lock(lhs, rhs);
        /* assign data */
        return *this;
    }
};
 
int main()
{
    R r;
}
```
