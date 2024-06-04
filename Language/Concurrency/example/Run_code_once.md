# 只运行代码一次

标准库提供了`call_once`函数可以保证只运行某个函数一次，本节就来分析一下它的主要部分的代码。

```CPP
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

以下是几个注意点

* 测试`__once._M_has_run`的`bool`值应该是线程安全的，否则如果一个线程正在测试这个值，其它线程修改了这个值，就会破坏运行，所以通常声明为`std::atomic_bool _M_has_run`.
* 获取了`_M_mutex`后，还需要测试`__once._M_has_run`，否则，由于`__f`的执行需要时间，可能会有多个线程同时进入，等待获取`_M_mutex`，为保证其它进入的线程可以看到`_M_has_run`已改变，所以需要测试`__once._M_has_run`.
* `__once._M_has_runs = true`和`__f(std::forward<_Args>(__args)...)`不能交换，因为如果交换，别的线程检测`__once._M_has_run`时，就会过早返回，从而别的线程在`__f`还未执行完毕就运行到了之后的代码。
