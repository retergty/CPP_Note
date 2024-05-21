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

`thread`对象可能不代表任何线程（默认构造，被移动，`detach`,`join`后)，一个线程也可能不与任何`thread`对象相关联（`detach`)后。

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

### 构建线程

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

  传递给`f`的参数是值复制或值移动的，所以如果需要传递引用，需要进行包装。

  `f`的返回值总是会被忽略，可能需要用到`std::promise`或`std::async`来把返回值传递给父线程。