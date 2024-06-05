# condition_variable

条件变量(condition variable)提供了多个线程间传递信息的方法。

参考文档

* [condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable)

定义在头文件`<condition_variable>`中.

## 类定义

```CPP
class condition_variable;
```

## 描述

条件变量是一个同步原语，它通常和`std::mutex`一起使用，阻塞一个或者多个线程直到别的线程修改了条件变量并通知给`condition_variable`.

想要修改条件变量的线程必须：

* 获取到对应的`std::mutex`所有权（通常是通过`std::lock_guard`).
* 修改条件变量的值.
* 调用条件变量的成员函数`notify_one`或者是`notify_all`。（可以在释放了互斥锁之后做）。

哪怕是条件变量是原子的，为了可以正确地通知别的线程，也必须使用互斥锁来修改条件变量。

想要等待条件变量的线程必须：

* 获取对应的`std::mutex`的所有权，也就是写入这个条件变量的互斥锁，通过`std::unique_lock<std::mutex>`
* 检查条件变量的值是否已经更新并通知了。
* 调用`wait`，`wait_for`,`wair_until`成员函数（原子地释放互斥锁，并挂起线程直到满足要求或者超时，并原子地获取互斥锁）
* 检查条件变量的值，如果没有更新，就继续等待。

或者是：

* 获取对应的`std::mutex`的所有权，也就是写入这个条件变量的互斥锁，通过`std::unique_lock<std::mutex>`
* 使用谓词版本的`wait`，`wait_for`,`wair_until`成员函数（它会把上面三步合一）.

`std::condition_variable`只能与`std::unique_lock<std::mutex>`一起使用