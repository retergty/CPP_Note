# packaged_task

参考文档

* [packaged_task](https://en.cppreference.com/w/cpp/thread/packaged_task)

## 类原型

```CPP
template< class > class packaged_task; //not defined
template< class R, class ...ArgTypes >
class packaged_task<R(ArgTypes...)>;
```

## 描述

`std::packaged_task`包装了任何可调用对象，使得它可以异步地运行，它的返回值或者是抛出的异常会存储在一个共享状态中，这个共享状态可以使用`std::future`访问。

## 成员函数

### 构造与析构

* [packaged_task](https://en.cppreference.com/w/cpp/thread/packaged_task/packaged_task)

  ```CPP
  packaged_task() noexcept;
  template< class F >
  explicit packaged_task( F&& f );
  packaged_task( const packaged_task& ) = delete;
  packaged_task( packaged_task&& rhs ) noexcept;
  ```

  `1`构造一个空的对象，没有共享状态。

  `2`构造一个对象，包含一个共享状态与调用任务的复制，这个任务的复制版本使用`std::forward<F>(f)`初始化。如果`f`本身与`f`的复制调用行为不一致，代码行为未定义。

  `3`禁止复制构造

  `4`移动构造函数，函数运行完毕后，`rhs`不再有任何的共享状态，并把任务的复制版本移动到`*this`上。

* [~packaged_task](https://en.cppreference.com/w/cpp/thread/packaged_task/%7Epackaged_task)

  ```CPP
  ~packaged_task();
  ```

  放弃共享状态并销毁存储的任务对象。

  如同`std::promise`析构函数一样，如果共享状态没有准备完成，将`std::future_error`异常使用代码`std::future_errc::broken_promise`存储在共享状态中，做好准备并释放。

* [operator=](https://en.cppreference.com/w/cpp/thread/packaged_task/operator%3D)

  ```CPP
  packaged_task& operator=( const packaged_task& ) = delete;
  packaged_task& operator=( packaged_task&& rhs ) noexcept;
  ```

  `1`禁止复制赋值。

  `2`移动赋值。

* [valid](https://en.cppreference.com/w/cpp/thread/packaged_task/valid)

  ```CPP
  bool valid() const noexcept;
  ```

  检查是否`*this`有共享状态。

* [swap](https://en.cppreference.com/w/cpp/thread/packaged_task/swap)

  ```CPP
  void swap( packaged_task& other ) noexcept;
  ```

  交换两个`packaged_task`.

### 获取结果

* [get_future](https://en.cppreference.com/w/cpp/thread/packaged_task/get_future)

  ```CPP
  std::future<R> get_future();
  ```

  返回一个`future`对象，与`*this`共享同一个共享状态。

  这个函数只能被调用一次。

### 处理

* [operator()](https://en.cppreference.com/w/cpp/thread/packaged_task/operator())

  ```CPP
  void operator()( ArgTypes... args );
  ```

  调用所存储的可调用对象，等价于`INVOKE<R>(f, args...)`。函数的返回值与异常会被存储在共享状态中，随后立即将共享状态准备完成，解锁所有因此而阻塞的线程。

* [make_ready_at_thread_exit](https://en.cppreference.com/w/cpp/thread/packaged_task/make_ready_at_thread_exit)

  ```CPP
  void make_ready_at_thread_exit( ArgTypes... args );
  ```

  调用所存储的可调用对象，等价于`INVOKE<R>(f, args...)`。函数的返回值与异常会被存储在共享状态中，当只有才线程退出，所有的线程本地的变量都销毁后，才将共享状态准备完成。

* [reset](https://en.cppreference.com/w/cpp/thread/packaged_task/reset)

  ```CPP
  void reset();
  ```

  重置状态，放弃先前执行的结果,新的共享状态被构建。

  等价于`*this = packaged_task(std::move(f))`.

## 推导指南

* [deduction guides for std::packaged_task](https://en.cppreference.com/w/cpp/thread/packaged_task/deduction_guides)

## 例子

```CPP
#include <cmath>
#include <functional>
#include <future>
#include <iostream>
#include <thread>
 
// unique function to avoid disambiguating the std::pow overload set
int f(int x, int y) { return std::pow(x, y); }
 
void task_lambda()
{
    std::packaged_task<int(int, int)> task([](int a, int b)
    {
        return std::pow(a, b); 
    });
    std::future<int> result = task.get_future();
 
    task(2, 9);
 
    std::cout << "task_lambda:\t" << result.get() << '\n';
}
 
void task_bind()
{
    std::packaged_task<int()> task(std::bind(f, 2, 11));
    std::future<int> result = task.get_future();
 
    task();
 
    std::cout << "task_bind:\t" << result.get() << '\n';
}
 
void task_thread()
{
    std::packaged_task<int(int, int)> task(f);
    std::future<int> result = task.get_future();
 
    std::thread task_td(std::move(task), 2, 10);
    task_td.join();
 
    std::cout << "task_thread:\t" << result.get() << '\n';
}
 
int main()
{
    task_lambda();
    task_bind();
    task_thread();
}
```
