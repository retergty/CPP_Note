# 互斥量

互斥量(mutex)避免不同线程同时访问共享资源，避免的竞争，并提供了线程间的同步功能。

## mutex

参考文档

* [mutex](https://en.cppreference.com/w/cpp/thread/mutex)

定义在头文件`<mutex>`中。

### mutex类原型

```CPP
class mutex;
```

### mutex类描述

`mutex`类是同步原语，可以被用于保护共享资源，防止竞争。

`mutex`提供独占的、非递归的所有权语义：

* 当一个调用线程成功调用`lock`或`try_lock`后，它获得`mutex`的所有权直到它调用`unlock`.