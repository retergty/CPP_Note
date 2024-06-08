# async

参考文档

* [async](https://en.cppreference.com/w/cpp/thread/async)

定义在头文件`<future>`中。

## 函数原型

```CPP
template< class F, class... Args >
std::future<std::invoke_result_t<std::decay_t<F>,
                                 std::decay_t<Args>...>>
    async( F&& f, Args&&... args );

template< class F, class... Args >
std::future<std::invoke_result_t<std::decay_t<F>,
                                 std::decay_t<Args>...>>
    async( std::launch policy, F&& f, Args&&... args );
```

## 描述

函数`std::async`异步地运行`f`(可能在一个单独的线程中，该线程可能是线程池的一部分),并返回`std::future`最终保存函数运行返回值。

`1`和`2`的行为一样，只是默认了`std::launch`为`std::launch::async | std::launch::deferred`.

`2`根据特定的`std::launch`调用函数`f`并传递参数`args`.

和`thread`构造函数一样，函数也是通过`INVOKE(decay-copy(std::forward<F>(f)),decay-copy(std::forward<Args>(args))...)`调用函数的，传递给`f`的参数是值复制或值移动的，所以如果需要传递引用，需要进行包装。

如果返回的`std::future`没有被移动到一个左值，或者绑定到一个引用上，则`std::future`的析构函数会阻塞当前线程直到异步操作完成，比如。

```CPP
std::async(std::launch::async, []{ f(); }); // temporary's dtor waits for f()
std::async(std::launch::async, []{ g(); }); // does not start until f() completes
```

注意，只有通过`async`函数获取的`future`析构函数才有可能会阻塞。

## Launch policies

### Async invocation

如果设置了异步处理，也就是`(policy & std::launch::async) != 0)`,则`async`则会好像是在一个`thread`中运行`INVOKE(decay-copy(std::forward<F>(f)),decay-copy(std::forward<Args>(args))...)`。

如果`f`返回了值或者抛出异常，则把它存储在对应的`std::future`中。

### Deferred invocation

如果设置了延迟处理，也就是`(policy & std::launch::deferred) != 0)`，则`async`会把`decay-copy(std::forward<F>(f))`与`decay-copy(std::forward<Args>(args))...`存储在共享状态中。

延迟求值发生在:

* 在对应的`future`上首次调用没有等待时间的`wait`成员函数，则会在当前线程调用`INVOKE(std::move(g), std::move(xyz))`,注意，这个线程不需要是当初调用`async`的线程，其中`g`就是`decay-copy(std::forward<F>(f))`,`xyz`就是`decay-copy(std::forward<Args>(args))...`.
* 结果或异常被放置在与返回的`std::future`关联的共享状态中，且之后共享状态准备完成（比如同时设置了异步处理，且操作系统选择了异步处理）。对同一`std::future`的所有进一步访问都将立即返回结果。

### Policy selection

如果超过一个方法被选择，则由实现来决定使用哪种方法。对于默认的情况(`std::launch::async | std::launch::deferred`)，标准推荐利用可用的并发同时推迟其它任务。

如果使用了`std::launch::async`则：

* 对`future`调用的等待共享状态的函数会阻塞线程直到对应的处理函数线程完成或者超时，且处理函数线程完成与等待共享状态的第一个函数的成功返回同步，或者与最后一个释放共享状态的函数的返回同步，以先到者为准。

## 例子

```CPP
#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>
 
std::mutex m;
 
struct X
{
    void foo(int i, const std::string& str)
    {
        std::lock_guard<std::mutex> lk(m);
        std::cout << str << ' ' << i << '\n';
    }
 
    void bar(const std::string& str)
    {
        std::lock_guard<std::mutex> lk(m);
        std::cout << str << '\n';
    }
 
    int operator()(int i)
    {
        std::lock_guard<std::mutex> lk(m);
        std::cout << i << '\n';
        return i + 10;
    }
};
 
template<typename RandomIt>
int parallel_sum(RandomIt beg, RandomIt end)
{
    auto len = end - beg;
    if (len < 1000)
        return std::accumulate(beg, end, 0);
 
    RandomIt mid = beg + len / 2;
    auto handle = std::async(std::launch::async,
                             parallel_sum<RandomIt>, mid, end);
    int sum = parallel_sum(beg, mid);
    return sum + handle.get();
}
 
int main()
{
    std::vector<int> v(10000, 1);
    std::cout << "The sum is " << parallel_sum(v.begin(), v.end()) << '\n';
 
    X x;
    // Calls (&x)->foo(42, "Hello") with default policy:
    // may print "Hello 42" concurrently or defer execution
    auto a1 = std::async(&X::foo, &x, 42, "Hello");
    // Calls x.bar("world!") with deferred policy
    // prints "world!" when a2.get() or a2.wait() is called
    auto a2 = std::async(std::launch::deferred, &X::bar, x, "world!");
    // Calls X()(43); with async policy
    // prints "43" concurrently
    auto a3 = std::async(std::launch::async, X(), 43);
    a2.wait();                     // prints "world!"
    std::cout << a3.get() << '\n'; // prints "53"
} // if a1 is not done at this point, destructor of a1 prints "Hello 42" here
```
