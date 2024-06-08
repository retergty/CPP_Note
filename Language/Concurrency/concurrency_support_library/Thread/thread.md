# thread

参考文档

* [std::thread](https://en.cppreference.com/w/cpp/thread/thread)

定义在`<thread>`头文件中。

## 类原型

```CPP
class thread;
```

## 描述

`thread`类代表了单个线程。

线程在它对应的`thread`类构造完毕后，从作为参数传递给`thread`类构造函数的执行函数立即开始执行，当然还需等待操作系统的调度延迟，但是这个取决于操作系统，不在`C++`负责范围内。执行函数可以传递它的返回值或者异常给父线程通过`std::promise`,或者修改共享资源（可能需要同步）。

`thread`对象可能不代表任何线程（默认构造，被移动，`detach`,`join`后），一个线程也可能不与任何`thread`对象相关联（`detach`）后。

两个`thread`对象**不可能**代表同一个线程，也就是说，`thread`对象不能复制可构造(CopyConstructible)与复制可赋值(CopyAssignable),但是可以移动可构造(MoveConstructible)与移动可赋值(MoveAssignable).

## 成员类型

* `native_handle_type`是可选的，表示本机的句柄类型，这个类型是由各个操作系统定义的。

## 成员类

* [id](https://en.cppreference.com/w/cpp/thread/thread/id)

  ```CPP
  class thread::id;
  ```

  代表这个`thread`类的独有的`id`.

  这个类是用于在关联容器中作为关键字的。

## 成员函数

### 线程对象构建与析构

* [thread](https://en.cppreference.com/w/cpp/thread/thread/thread)

  ```CPP
  thread() noexcept;
  thread( thread&& other ) noexcept;
  template< class F, class... Args >
  explicit thread( F&& f, Args&&... args );
  thread( const thread& ) = delete;
  ```

  类构造函数

  `1`构建一个新的`thread`类，不表示任何的线程

  `2`移动构造函数，构建`thread`类，并使得它代表`other`所代表的线程。调用完毕后，`other`不再表示任何线程。

  `3`创建一个新的`thread`类，并把它关联到一个线程，开始新线程的执行，通过调用`INVOKE(decay-copy(std::forward<F>(f)),decay-copy(std::forward<Args>(args))...)`.完成`thread`类的构造函数后，才会在新的线程开始执行`f`的副本（同步）。

  `4`禁止复制构造函数

  传递给`f`的参数是值复制或值移动的，所以如果需要传递引用，需要进行包装。`decay-copy`返回`std::decay<T>::type`，就是去除了引用后的类型。比如对于`int n`,变为`decay-copy(std::forward<int&>(n))`,变为`std::decay<int&>::type`,变为`std::remove_reference<int&>::type`,变为`int`也就是说`decay-copy`返回了`prvalue`.

  `f`的返回值总是会被忽略，可能需要用到`std::promise`或`std::async`来把返回值传递给父线程。

* [~thread](https://en.cppreference.com/w/cpp/thread/thread/%7Ethread)

  ```CPP
  ~thread();
  ```

  销毁`thread`对象。

  如果`*this`有一个关联的对象(`joinable() == true`),就会调用`std::terminate()`.

* [operator=](https://en.cppreference.com/w/cpp/thread/thread/operator%3D)

  ```CPP
  thread& operator=( thread&& other ) noexcept;
  ```

  移动赋值函数，如果`*this`仍然有相关联的线程，(`joinable() == true`),调用`std::terminate()`停止这个线程。之后将`other`的状态移动给`*this`,设置`other`为默认构造时的状态。

  调用完毕后，`this->get_id()`与`other.get_id()`,`ohter`不再代表一个执行线程。

### 获得线程状态

* [joinable](https://en.cppreference.com/w/cpp/thread/thread/joinable)

  ```CPP
  bool joinable() const noexcept;
  ```

  检查是否`thread`对象代表了一个活跃的线程.特别地，返回`true`如果`get_id() != std::thread::id()`.所以，默认构造的`thread`对象不认为是`joinable`的。

  如果一个线程完成了处理函数，但是还没有被`join`，仍然认为线程是活跃的。

* [get_id](https://en.cppreference.com/w/cpp/thread/thread/get_id)

  ```CPP
  std::thread::id get_id() const noexcept;
  ```

  返回与`*this`相关联的线程的独有`id`,如果`*this`不与任何线程相关联，返回`std::thread::id`的默认构造版本。

* [native_handle](https://en.cppreference.com/w/cpp/thread/thread/native_handle)

  ```CPP
  native_handle_type native_handle();
  ```

  返回操作系统相关的句柄。

* [hardware_concurrency](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency)

  ```CPP
  static unsigned int hardware_concurrency() noexcept;
  ```

  返回支持的并发线程最大数量，这个值只是一个提示，不保证正确性。

### 线程操作

* [join](https://en.cppreference.com/w/cpp/thread/thread/join)

  ```CPP
  void join();
  ```

  阻塞当前线程，直到`*this`关联的线程结束运行。

  `*this`关联的线程与`join()`函数返回点同步。

  不会对`*this`本身同步。不同线程调用同一个`thread`对象的`join()`函数会带来竞争。

* [detach](https://en.cppreference.com/w/cpp/thread/thread/detach)

  ```CPP
  void detach();
  ```

  将线程与`thread`对象分离开，线程独立执行。任何分配的资源会在线程退出后释放。

  这个函数调用完毕后，`*this`不再与任何线程相关联。这个函数可以防止当`thread`对象结束生命周期时，仍需要线程运行。

* [swap](https://en.cppreference.com/w/cpp/thread/thread/swap)

  ```CPP
  void swap( std::thread& other ) noexcept;
  ```

  交换两个`thread`对象。

## 操作当前线程的函数

有一系列函数，专门用于操作当前线程。定义在头文件`<thread>`中的名称空间`this_thread`中

* [yield](https://en.cppreference.com/w/cpp/thread/yield)

  ```CPP
  void yield() noexcept;
  ```

  提示操作系统，放弃当前线程的执行权，让操作系统运行调度器。

  这个函数确切的功能取决于操作系统实现，特别是`OS`调度器的实现，比如对于`FIFO`实时调度器，(比如linux上的`SCHED_FIFO`)，会挂起当前的线程并放置在相同优先级的队列的末尾。

* [get_id](https://en.cppreference.com/w/cpp/thread/get_id)

  ```CPP
  std::thread::id get_id() noexcept;
  ```

  返回当前的线程`id`.

* [sleep_for](https://en.cppreference.com/w/cpp/thread/sleep_for)

  ```CPP
  template< class Rep, class Period >
  void sleep_for( const std::chrono::duration<Rep, Period>& sleep_duration );
  ```

  阻塞当前的线程，等待**至少**`sleep_duration`后恢复。

  这个函数可能会等待超过`sleep_duration`时间，因为调度器或者资源的延迟。

* [sleep_until](https://en.cppreference.com/w/cpp/thread/sleep_until)

  ```CPP
  template< class Clock, class Duration >
  void sleep_until( const std::chrono::time_point<Clock, Duration>& sleep_time );
  ```

  阻塞当前的进程，直到`sleep_time`到达。
