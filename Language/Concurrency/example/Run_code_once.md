# 只运行函数一次

## 所有线程都可以观测到函数运行的副作用

标准库提供了`call_once`函数可以保证只运行某个函数一次并且任何线程在`call_once`成功返回后，可以认为函数已经运行完毕，本节就来分析一下它的主要部分的代码。

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

## 线程不需要观测到函数运行的副作用

如果别的线程不需要观测到函数运行的副作用，那么可以不需要互斥锁。

```CPP
#include <iostream>
#include <thread>
#include <atomic>
#include <unistd.h>

using namespace std;
using my_once_flag = atomic<bool>;

void my_call_once(my_once_flag& flag, std::function<void()> foo) {
    bool expected = false;
    bool res = flag.compare_exchange_strong(expected, true,
                                            std::memory_order_release, std::memory_order_relaxed);
    if(res)
        foo();
}
my_once_flag flag;
void printOnce() {
    usleep(100);
    my_call_once(flag, [](){
       cout << "test" << endl;
    });
}

int main() {
    for(int i = 0; i< 500; ++i){
            thread([](){
                printOnce();
            }).detach();
    }
    return 0;
} 
```

以下是几个注意点

* 对于`flag`进行原子操作，保证在多个线程里，`flag`只被翻转一次，且返回`true`.
* 但是可能在运行`foo`时，其它线程已经运行到了后面，所以观测不到函数运行的副作用。
