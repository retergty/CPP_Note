# recursive_mutex

参考文档

* [recursive_mutex](https://en.cppreference.com/w/cpp/thread/recursive_mutex)

## 类原型

```CPP
class recursive_mutex;
```

定义在头文件`<mutex>`中。

## 描述

`recursive_mutex`类是一个同步原语，可以被用在保护共享资源防止竞争。

`recursive_mutex`类提供一个独占，递归的所有权语义：

* 当一个调用线程成功调用`lock`或`try_lock`后，它获得`recursive_mutex`的所有权，这个线程可以多次调用`lock`或`try_lock`，调用线程调用`unlock`到达之前调用`lock`或`unlock`的次数后，就释放`recursive_mutex`的所有权。
* 当一个线程获得`mutex`的所有权后，所有其它的线程会在调用`lock`时阻塞或者是调用`try_lock`时返回`false`.
* 可以调用`recursive_mutex`的`lock`的最大次数未指定，当保证到达最大次数后，接下来的`lock`会抛出异常，`try_lock`会返回`false`.

如果在`recursive_mutex`类被析构时，它仍被某个线程所占用或者是当某个线程终止时，仍占有`recursive_mutex`，则程序未定义。

`recursive_mutex`类不是可复制和可移动的。

## 成员类定义

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## 成员函数

### 构造与析构

* [recursive_mutex](https://en.cppreference.com/w/cpp/thread/recursive_mutex/recursive_mutex)

  ```CPP
  recursive_mutex();
  recursive_mutex( const recursive_mutex& ) = delete;
  ```

  构造`recursive_mutex`类，构造完毕后，`recursive_mutex`类处于`unlock`状态。

### 锁定与解锁

* [lock](https://en.cppreference.com/w/cpp/thread/mutex/lock)

  ```CPP
  void lock();
  ```

  锁定`recursive_mutex`，如果其它的线程已经锁定了这个`recursive_mutex`，这个调用会使得这个线程被阻塞直到可以获得这个`recursive_mutex`的所有权为止。

  `lock`会增加所有权计数。

  线程可以多次锁定`recursive_mutex`,所有权会在`unlock`达到了`lock`加上`try_lock`的次数后释放。

  在同一个`recursive_mutex`上之前的`unlock`操作与`lock`同步，(std::memory_order中的定义).

  `lock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。

* [try_lock](https://en.cppreference.com/w/cpp/thread/recursive_mutex/try_lock)

  ```CPP
  bool try_lock() noexcept;
  ```

  尝试锁定`recursive_mutex`,立即返回，如果成功获得`recursive_mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`recursive_mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  成功的`try_lock`会增加所有权计数。

  线程可以多次锁定`recursive_mutex`,所有权会在`unlock`达到了所有权计数的次数后释放。

  如果返回`true` 在同一个`mutex`上之前的`unlock`操作与`try_lock`同步.

* [unlock](https://en.cppreference.com/w/cpp/thread/recursive_mutex/unlock)

  ```CPP
  void unlock();
  ```

  递减所有权计数，并在所有权计数为零后解锁`recursive_mutex`

  当前线程必须已经锁定了这个`recursive_mutex`,否则程序行为未定义。

  这个操作与接下来的`lock`相同`mutex`的序列同步。

  `unlock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。

## 使用recursive_mutex保护共享资源

```CPP
#include <iostream>
#include <mutex>
#include <thread>
 
class X
{
    std::recursive_mutex m;
    std::string shared;
public:
    void fun1()
    {
        std::lock_guard<std::recursive_mutex> lk(m);
        shared = "fun1";
        std::cout << "in fun1, shared variable is now " << shared << '\n';
    }
    void fun2()
    {
        std::lock_guard<std::recursive_mutex> lk(m);
        shared = "fun2";
        std::cout << "in fun2, shared variable is now " << shared << '\n';
        fun1(); // recursive lock becomes useful here
        std::cout << "back in fun2, shared variable is " << shared << '\n';
    }
};
 
int main() 
{
    X x;
    std::thread t1(&X::fun1, &x);
    std::thread t2(&X::fun2, &x);
    t1.join();
    t2.join();
}
```
