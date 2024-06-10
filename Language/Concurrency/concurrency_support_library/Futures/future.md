# future

参考文档

* [future](https://en.cppreference.com/w/cpp/thread/future)

## 类原型

```CPP
template< class T > class future;
template< class T > class future<T&>;
template<> class future<void>;
```

## 描述

`std::future`提供了一个获取异步操作结果的机制：

* 异步操作(`std::async`,`std::packaged_task`,或者`std::promise`)可以提供一个`std::future`对象。
* 可以使用这个对象进行等待，询问或者提取出对应的值。如果异步操作还未完成，这些方法会阻塞当前线程。
* 当异步操作已经准备好并存储了结果，通过修改与`std::future`相连的共享状态(比如`std::promise::set_value`)，

注意，与`std::shared_future`相反，`std::future`引用的共享状态不会与其它的异步操作返回对象所共享,也就是说，不能有两个`std::future`引用同一个共享状态。

## 成员函数

### 构造与析构

* [future](https://en.cppreference.com/w/cpp/thread/future/future)

  ```CPP
  future() noexcept;
  future( future&& other ) noexcept;
  future( const future& other ) = delete;
  ```

  `1`默认构造函数，构造`std::future`，不指向任何共享状态。构造完毕后`valid() == false`

  `2`移动构造函数，构造完毕后，`other.valid() == false`

  `3`禁止复制构造

* [~future](https://en.cppreference.com/w/cpp/thread/future/%7Efuture)

  ```CPP
  ~future();
  ```

  释放引用的共享状态，并销毁`std::future`.

  * 如果`*this`是最后一个引用这个共享状态的对象，销毁这个共享状态
  * `*this`放弃对这个共享状态的引用。
  * 这些操作不会阻塞线程直到共享状态准备好，但在以下情况，可能会阻塞线程
    * 这个共享状态是通过`std::async`函数创建的
    * 这个共享状态还没有准备完成，且`*this`是这个共享状态的最后一个引用。
  
  在实践上，这些操作只有当任务的启动策略是`std::launch::async`（参见“Effective Modern C++”第 36 条）时才会阻塞当前线程，启动策略可能是由运行时操作系统指定的或者是在`std::async`函数调用时指定的。

* [operator=](https://en.cppreference.com/w/cpp/thread/future/operator%3D)

  ```CPP
  future& operator=( future&& other ) noexcept;
  future& operator=( const future& other ) = delete;
  ```

* [share](https://en.cppreference.com/w/cpp/thread/future/share)

  ```CPP
  std::shared_future<T> share() noexcept;
  ```

  将`*this`引用的共享状态（如果有的话）传递给`std::shared_future`对象。多个`std::shared_future`可能指向同一个共享状态，但是`std::future`则不可能。

  调用完毕后`valid() == false`.

### 得到结果

* [get](https://en.cppreference.com/w/cpp/thread/future/get)

  ```CPP
  Main template
  T get();
  ```

  ```CPP
  std::future<T&> specializations
  T& get();
  ```

  ```CPP
  std::future<void> specialization
  void get();
  ```

  等待(通过内部调用`wait()`)直到共享状态准备完成，然后获取存储在共享状态中的值。调用完毕后，`valid()`变为`false`.这是因为只能获取一次共享状态的值。

  如果在`valud()`为`false`时调用这个函数，则函数行为未定义。

  `1`返回存储在共享状态的值，通过`std::move(v)`.

  `2`返回存储在共享状态里的引用。

  `3`返回空。

### 状态

* [valid](https://en.cppreference.com/w/cpp/thread/future/valid)

  ```CPP
  bool valid() const noexcept;
  ```

  检查是否`*this`引用一个共享状态。

  只有在`*this`不是默认构造或者是被移动的情况，也就是说是通过函数`std::promise::get_future()`,`std::packaged_task::get_future()`或`std::async()`,直到第一个`get()`或者`share()`调用前的状态。

  如果是除了析构函数，移动运算符的其它成员函数在`*this`上调用，而`*this`实际上没有引用的共享状态，这个函数行为未定义。

* [wait](https://en.cppreference.com/w/cpp/thread/future/wait)

  ```CPP
  void wait() const;
  ```

  等待共享状态准备完成，阻塞当前线程，调用完毕后`valid() == true`.

  如果调用前，`valid() == false`函数行为未定义。

* [wait_for](https://en.cppreference.com/w/cpp/thread/future/wait_for)

  ```CPP
  template< class Rep, class Period >
  std::future_status wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) const;
  ```

  等待共享状态准备完成，阻塞当前线程，具有超时机制，返回值表示了等待的结果。

  如果`std::future`是函数`std::async`使用延迟求值的返回结果，则函数立即返回。

  如果调用前，`valid() == false`函数行为未定义。

* [wait_until](https://en.cppreference.com/w/cpp/thread/future/wait_until)

  ```CPP
  template< class Clock, class Duration >
  std::future_status wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const;
  ```

  等待共享状态准备完成，阻塞当前线程，具有超时机制，返回值表示了等待的结果。

  如果`std::future`是函数`std::async`使用延迟求值的返回结果，则函数立即返回。

  如果调用前，`valid() == false`函数行为未定义。

## future_status类

`std::future_status`是成员函数`wait_for`和`wait_until`的返回类型，它是一个枚举类。

```CPP
enum class future_status {
    ready,
    timeout,
    deferred
};
```

* `future_status::ready`表示返回时共享状态已准备好
* `future_status::timeout`表示返回时共享状态没有准备好
* `future_status::deferred`表示共享状态包含一个延迟求值的函数，所以只会在显式需求时计算。
