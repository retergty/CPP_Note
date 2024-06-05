# mutex

互斥量(mutex)避免不同线程同时访问共享资源，提供了独占执行(mutual exclusion)，避免的竞争，并提供了线程间的同步功能。

参考文档

* [mutex](https://en.cppreference.com/w/cpp/thread/mutex)

定义在头文件`<mutex>`中。

## mutex类原型

```CPP
class mutex;
```

## mutex类描述

`mutex`类是同步原语，可以被用于保护共享资源，防止竞争。

`mutex`提供独占的、非递归的所有权语义：

* 当一个调用线程成功调用`lock`或`try_lock`后，它获得`mutex`的所有权直到它调用`unlock`.
* 当一个线程获得`mutex`的所有权后，所有其它的线程会在调用`lock`时阻塞或者是调用`try_lock`时返回`false`.
* 在调用`lock`或`try_lock`之前，调用线程不应该有这个`mutex`的所有权。

如果在`mutex`类被析构时，它仍被某个线程所占用或者是当某个线程终止时，仍占有`mutex`，则程序未定义。

`mutex`类不是可复制和可移动的。

## mutex类成员类定义

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## mutex类成员函数

### 构造mutex类

* [mutex](https://en.cppreference.com/w/cpp/thread/mutex/mutex)

  ```CPP
  constexpr mutex() noexcept;
  mutex( const mutex& ) = delete;
  ```

  构造`mutex`类，构造完毕后，`mutex`类处于`unlock`状态。

### 锁定与解锁mutex

* [lock](https://en.cppreference.com/w/cpp/thread/mutex/lock)

  ```CPP
  void lock();
  ```

  锁定`mutex`，如果其它的线程已经锁定了这个`mutex`，这个调用会使得这个线程被阻塞直到可以获得这个`mutex`的所有权为止。

  如果这个线程已经有了这个`mutex`的所有权，程序行为未定义。

  在同一个`mutex`上之前的`unlock`操作与`lock`同步，(std::memory_order中的定义).

  `lock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。

* [try_lock](https://en.cppreference.com/w/cpp/thread/mutex/try_lock)

  ```CPP
  bool try_lock();
  ```

  尝试锁定`mutex`,立即返回，如果成功获得`mutex`的所有权就返回`true`,否则返回`false`.

  哪怕是`mutex`当前没有被任何其他线程锁定，该函数也允许虚假失败并返回`false`。

  如果这个线程已经有了这个`mutex`的所有权，程序行为未定义。

  如果返回`true` 在同一个`mutex`上之前的`unlock`操作与`try_lock`同步.

* [unlock](https://en.cppreference.com/w/cpp/thread/mutex/unlock)

  ```CPP
  void unlock();
  ```

  解锁`mutex`。

  当前线程必须已经锁定了这个`mutex`,否则程序行为未定义。

  这个操作与接下来的`lock`相同`mutex`的序列同步。

  `unlock`函数通常不是直接调用的，`std::unique_lock`,`std::scoped_lock`,`std::lock_guard`用于管理互斥锁。

## 使用mutex保护共享资源

这个例子显示了使用`mutex`保护`std::map`的例子。

```CPP
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
 
std::map<std::string, std::string> g_pages;
std::mutex g_pages_mutex;
 
void save_page(const std::string& url)
{
    // simulate a long page fetch
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::string result = "fake content";
 
    std::lock_guard<std::mutex> guard(g_pages_mutex);
    g_pages[url] = result;
}
 
int main() 
{
    std::thread t1(save_page, "http://foo");
    std::thread t2(save_page, "http://bar");
    t1.join();
    t2.join();
 
    // safe to access g_pages without lock now, as the threads are joined
    for (const auto& [url, page] : g_pages)
        std::cout << url << " => " << page << '\n';
}
```
