# call_once

参考文档

* [call_once](https://en.cppreference.com/w/cpp/thread/call_once)

定义在`<mutex>`.

## 函数原型

```CPP
template< class Callable, class... Args >
void call_once( std::once_flag& flag, Callable&& f, Args&&... args );
```

## 描述

只会调用可调用对象`f`一次，哪怕是`call_once`出现在多个线程。

具体来说，就是：

* 如果在`call_once`调用时，`flag`显示`f`早已被调用过，则`call_once`立即返回，（这种类型的调用被称为`passive`)
* 否则，`call_once`调用`INVOKE(std::forward<Callable>(f), std::forward<Args>(args)...)`.不同于`std::thread`构造函数，参数不会复制或者移动，因为它们不需要被传递到其它线程.（这种类型的调用被称为`active`）.其中，如果发生了异常，`flag`不会翻转,所以其它的`call_once`可以尝试运行`f`；如果正常运行，则翻转`flag`,表示`f`已经调用过一次。

在相同`flag`上所有的`active`调用组成了一个顺序，每个`active`调用的结束与下一个`active`调用同步。

所有的并行调用`call_once`保证可以观测到任何`active`调用带来的副作用，不需要额外的同步步骤。也就是说，当`call_once`函数返回时，我们可以保证`f`只调用一次，且`f`所有的副作用都可以被观测到（比如初始化变量，分配内存等）。

注意，函数的`static`变量保证只会初始化一次，哪怕是在多个线程调用这个函数，也许会比`call_once`要更快。

## 例子

```CPP
#include <iostream>
#include <mutex>
#include <thread>
 
std::once_flag flag1, flag2;
 
void simple_do_once()
{
    std::call_once(flag1, [](){ std::cout << "Simple example: called once\n"; });
}
 
void may_throw_function(bool do_throw)
{
    if (do_throw)
    {
        std::cout << "Throw: call_once will retry\n"; // this may appear more than once
        throw std::exception();
    }
    std::cout << "Did not throw, call_once will not attempt again\n"; // guaranteed once
}
 
void do_once(bool do_throw)
{
    try
    {
        std::call_once(flag2, may_throw_function, do_throw);
    }
    catch (...) {}
}
 
int main()
{
    std::thread st1(simple_do_once);
    std::thread st2(simple_do_once);
    std::thread st3(simple_do_once);
    std::thread st4(simple_do_once);
    st1.join();
    st2.join();
    st3.join();
    st4.join();
 
    std::thread t1(do_once, true);
    std::thread t2(do_once, true);
    std::thread t3(do_once, false);
    std::thread t4(do_once, true);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
```

## 可能实现

本节给出了`call_once`的主要部分的可能实现，使用的多线程编程方法可能有所帮助。

```CPP
struct once_flag {
private:
    std::mutex _M_mutex;
    std::atomic_bool _M_has_run;
public:
    /// Default constructor
    once_flag() : _M_has_run(false) {}

    /// Deleted copy constructor
    once_flag(const once_flag&) = delete;
    /// Deleted assignment operator
    once_flag& operator=(const once_flag&) = delete;

    template<typename _Callable, typename... _Args>
    friend void
    call_once(once_flag& __once, _Callable&& __f, _Args&&... __args);
};

/// call_once
template<typename _Callable, typename... _Args>
void
call_once(once_flag& __once, _Callable&& __f, _Args&&... __args)
{
    // Early exit without locking
    if(__once._M_has_run) return;
    unique_lock<mutex> __l(__once._M_mutex);
    // Check again now that we locked the mutex
    if(__once._M_has_run) return;
    __f(std::forward<_Args>(__args)...);
    __once._M_has_runs = true;
}
```
