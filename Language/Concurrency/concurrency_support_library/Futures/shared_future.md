# shared_future

参考文档

* [shared_future](https://en.cppreference.com/w/cpp/thread/shared_future)

定义在头文件`<future>`中

## 类原型

```CPP
template< class T > class shared_future;
template< class T > class shared_future<T&>;
template<> class shared_future<void>;
```

## 描述

`std::shared_future`提供了一个获取异步操作结果的机制，和`std::future`类型，只不过复数线程可以等待同一个共享状态。此外，不同于`std::future`,`std::shared_future`可以复制且多个可能指向同一个共享状态。

如果每个线程都使用它自己对`shared_future`的副本访问同一个共享状态，不需要额外的同步操作。

## 成员函数

### 构造与析构

* [shared_future](https://en.cppreference.com/w/cpp/thread/shared_future/shared_future)

  ```CPP
  shared_future() noexcept;
  shared_future( const shared_future& other ) noexcept;
  shared_future( std::future<T>&& other ) noexcept;
  shared_future( shared_future&& other ) noexcept;
  ```

  `1`默认构造函数，构造`std::shared_future`，不指向任何共享状态。构造完毕后`valid() == false`

  `2`复制构造函数，构造`std::shared_future`，指向与`other`相同的共享状态。

  `3`,`4`把`other`掌握的共享状态转换到`*this`.构造完毕后，`other.valid() == false`，且`this->valid()`返回与之前`other`相同的状态。

* [~shared_future](https://en.cppreference.com/w/cpp/thread/shared_future/%7Eshared_future)

  ```CPP
  ~shared_future();
  ```

  如果`*this`是最后一个指向某个共享状态的对象，销毁这个共享状态，否则，什么都不做。

* [operator=](https://en.cppreference.com/w/cpp/thread/shared_future/operator%3D)

  ```CPP
  shared_future& operator=( const shared_future& other ) noexcept;
  shared_future& operator=( shared_future&& other ) noexcept;
  ```

  `1`释放`*this`指向的共享状态，并将`other`复制给`*this`，复制完毕后，`this->valid() == other.valid()`.

  `2`移动赋值运算符。释放`*this`指向的共享状态，并将`other`移动给`*this`，

### 得到结果

* [get](https://en.cppreference.com/w/cpp/thread/shared_future/get)

  ```CPP
  Main template
  T get();
  ```

  ```CPP
  std::shared_future<T&> specializations
  T& get();
  ```

  ```CPP
  std::shared_future<void> specialization
  void get();
  ```

  等待(通过内部调用`wait()`)直到共享状态准备完成，然后获取存储在共享状态中的值。

  如果在`valud()`为`false`时调用这个函数，则函数行为未定义。

### 状态

* [valid](https://en.cppreference.com/w/cpp/thread/shared_future/valid)

  ```CPP
  bool valid() const noexcept;
  ```

  检查是否`*this`引用一个共享状态。

  只有在`*this`不是默认构造或者是被移动的情况，也就是说是通过函数`std::promise::get_future()`,`std::packaged_task::get_future()`或`std::async()`.不同于`std::future`,`std::shared_future`当`get()`调用完毕后，它的共享状态不会失效。

  如果是除了析构函数，移动运算符的其它成员函数在`*this`上调用，而`*this`实际上没有引用的共享状态，这个函数行为未定义。

* [wait](https://en.cppreference.com/w/cpp/thread/shared_future/wait)

  ```CPP
  void wait() const;
  ```

  等待共享状态准备完成，阻塞当前线程，调用完毕后`valid() == true`.

  如果调用前，`valid() == false`函数行为未定义。

* [wait_for](https://en.cppreference.com/w/cpp/thread/shared_future/wait_for)

  ```CPP
  template< class Rep, class Period >
  std::future_status wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) const;
  ```

  等待共享状态准备完成，阻塞当前线程，具有超时机制，返回值表示了等待的结果。

  如果`std::shared_future`是函数`std::async`使用延迟求值的返回结果，则函数立即返回。

  如果调用前，`valid() == false`函数行为未定义。

* [wait_until](https://en.cppreference.com/w/cpp/thread/shared_future/wait_until)

  ```CPP
  template< class Clock, class Duration >
  std::future_status wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const;
  ```

  等待共享状态准备完成，阻塞当前线程，具有超时机制，返回值表示了等待的结果。

  如果`std::shared_future`是函数`std::async`使用延迟求值的返回结果，则函数立即返回。

  如果调用前，`valid() == false`函数行为未定义。

## 例子

```CPP
#include <chrono>
#include <future>
#include <iostream>
 
int main()
{   
    std::promise<void> ready_promise, t1_ready_promise, t2_ready_promise;
    std::shared_future<void> ready_future(ready_promise.get_future());
 
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
 
    auto fun1 = [&, ready_future]() -> std::chrono::duration<double, std::milli> 
    {
        t1_ready_promise.set_value();
        ready_future.wait(); // waits for the signal from main()
        return std::chrono::high_resolution_clock::now() - start;
    };
 
 
    auto fun2 = [&, ready_future]() -> std::chrono::duration<double, std::milli> 
    {
        t2_ready_promise.set_value();
        ready_future.wait(); // waits for the signal from main()
        return std::chrono::high_resolution_clock::now() - start;
    };
 
    auto fut1 = t1_ready_promise.get_future();
    auto fut2 = t2_ready_promise.get_future();
 
    auto result1 = std::async(std::launch::async, fun1);
    auto result2 = std::async(std::launch::async, fun2);
 
    // wait for the threads to become ready
    fut1.wait();
    fut2.wait();
 
    // the threads are ready, start the clock
    start = std::chrono::high_resolution_clock::now();
 
    // signal the threads to go
    ready_promise.set_value();
 
    std::cout << "Thread 1 received the signal "
              << result1.get().count() << " ms after start\n"
              << "Thread 2 received the signal "
              << result2.get().count() << " ms after start\n";
}
```
