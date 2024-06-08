# promise

参考文档

* [promise](https://en.cppreference.com/w/cpp/thread/promise)

## 类原型

```CPP
template< class R > class promise;
template< class R > class promise<R&>;
template<> class promise<void>;
```

## 描述

`std::promise`提供了一个存储值或者异常，并随后可以被`std::promise`所创建的对象`std::future`异步地获取的机制。注意，`std::promise`被设计为只会被使用一次。

每个`promise`都与一个共享状态(shared state)相关联，这个共享状态包含一些状态信息与一个可能还未被处理的结果，这个结果可能是一个值(通常类型是void)或者是一个异常。一个`promise`可能会对共享状态做如下三件事：

* 做好准备(make ready),`promise`将结果存储在共享状态中。标记状态为准备完成，解锁所有等待的线程。
* 释放(release),`promise`放弃对共享状态的引用，如果这是最后一个对其的引用，则销毁共享状态对象。除非这个共享状态是通过`std::async`创建的尚未准备就绪的共享状态，否则此操作不会阻塞。
* 放弃(abandon),`promise`将`std::future_error`异常使用代码`std::future_errc::broken_promise`存储在共享状态中，做好准备并释放。

`promse`被压入到`promise-future`通信管道中，也就是说，将一个结果存储在共享状态的操作与任何等待共享状态的任何函数（例如 std::future::get）的成功返回同步。对同一共享状态的并发访问可能会发生冲突：例如`std::shared_future::get`的多个调用者必须全部是只读的或提供额外的同步。

## 成员函数

### 构造与析构

* [promise](https://en.cppreference.com/w/cpp/thread/promise/promise)

  ```CPP
  promise();
  template< class Alloc >
  promise( std::allocator_arg_t, const Alloc& alloc );
  promise( promise&& other ) noexcept;
  promise( const promise& other ) = delete;
  ```

  构建`promise`对象。

  `1`默认构造函数，构造`promise`，具有空的共享状态。

  `2`构造`promise`，具有空的共享状态，使用`alloc`分配共享状态的空间。

  `3`移动构造函数，移动完成后,`other`不再有共享状态。

  `4`禁止复制构造。

* [~promise](https://en.cppreference.com/w/cpp/thread/promise/%7Epromise)

  ```CPP
  ~promise();
  ```

  如果共享状态准备完成，释放它。

  如果共享状态没有准备完成，将`std::future_error`异常使用代码`std::future_errc::broken_promise`存储在共享状态中，做好准备并释放。

* [operator=](https://en.cppreference.com/w/cpp/thread/promise/operator%3D)

  ```CPP
  promise& operator=( promise&& other ) noexcept;
  promise& operator=( const promise& rhs ) = delete;
  ```

* [swap](https://en.cppreference.com/w/cpp/thread/promise/swap)

  ```CPP
  void swap( promise& other ) noexcept;
  ```

  交换两个`promise`对象的共享状态。

### 获取结果

* [get_future](https://en.cppreference.com/w/cpp/thread/promise/get_future)

  ```CPP
  std::future<R> get_future();
  ```

  返回一个与`*this`具有相同关联的共享状态的`std::future`对象。

  如果`*this`没有共享状态或者已经调用过`get_future`了，则抛出异常。为了得到复数个`promise-future`通信管道的信息，使用`std::future::share`.

  这个函数不会与`set_value`, `set_exception`, `set_value_at_thread_exit`, `set_exception_at_thread_exit`函数产生竞争，所以不需要额外的同步。

### 设置结果

* [set_value](https://en.cppreference.com/w/cpp/thread/promise/set_value)

  ```CPP
  Main template
  void set_value( const R& value );
  void set_value( R&& value );
  ```

  ```CPP
  std::promise<R&> specializations
  void set_value( R& value );
  ```

  ```CPP
  std::promise<void> specialization
  void set_value();
  ```

  `1`,`2`,`3`原子地把`value`存储在共享状态中，并使得共享状态准备好。

  `4`单纯使得共享状态准备好。

  这个操作行为好像是`set_value`, `set_exception`, `set_value_at_thread_exit`, 与`set_exception_at_thread_exit`在更新共享状态时获取了同一个互斥锁。

  调用这个函数不会与`get_future`产生竞争。

  当`*this`没有共享状态或者是共享状态早已存储了一个`value`或者异常，程序抛出异常。

* [set_value_at_thread_exit](https://en.cppreference.com/w/cpp/thread/promise/set_value_at_thread_exit)

  ```CPP
  Main template
  void set_value_at_thread_exit( const R& value );
  void set_value_at_thread_exit( R&& value );
  ```

  ```CPP
  std::promise<R&> specializations
  void set_value_at_thread_exit( R& value );
  ```

  ```CPP
  std::promise<void> specialization
  void set_value_at_thread_exit();
  ```

  将`value`存储在共享状态中，但是不立即将共享状态准备好，而是等到当前线程退出时准备好，在所有的线程变量析构后。

* [set_exception](https://en.cppreference.com/w/cpp/thread/promise/set_exception)

  ```CPP
  void set_exception( std::exception_ptr p );
  ```

  原子地把`p`存储在共享状态中，并使得共享状态准备好。

* [set_exception_at_thread_exit](https://en.cppreference.com/w/cpp/thread/promise/set_exception_at_thread_exit)

  ```CPP
  void set_exception_at_thread_exit( std::exception_ptr p );
  ```

  把`p`存储在共享状态中，但是不立即将共享状态准备好，而是等到当前线程退出时准备好，在所有的线程变量析构后。

## 例子

```CPP
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>
 
void accumulate(std::vector<int>::iterator first,
                std::vector<int>::iterator last,
                std::promise<int> accumulate_promise)
{
    int sum = std::accumulate(first, last, 0);
    accumulate_promise.set_value(sum); // Notify future
}
 
void do_work(std::promise<void> barrier)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    barrier.set_value();
}
 
int main()
{
    // Demonstrate using promise<int> to transmit a result between threads.
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6};
    std::promise<int> accumulate_promise;
    std::future<int> accumulate_future = accumulate_promise.get_future();
    std::thread work_thread(accumulate, numbers.begin(), numbers.end(),
                            std::move(accumulate_promise));
 
    // future::get() will wait until the future has a valid result and retrieves it.
    // Calling wait() before get() is not needed
    // accumulate_future.wait(); // wait for result
    std::cout << "result=" << accumulate_future.get() << '\n';
    work_thread.join(); // wait for thread completion
 
    // Demonstrate using promise<void> to signal state between threads.
    std::promise<void> barrier;
    std::future<void> barrier_future = barrier.get_future();
    std::thread new_work_thread(do_work, std::move(barrier));
    barrier_future.wait();
    new_work_thread.join();
}
```
