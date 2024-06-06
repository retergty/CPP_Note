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

