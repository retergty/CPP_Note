# recursive_mutex

参考文档

* [recursive_mutex](https://en.cppreference.com/w/cpp/thread/recursive_mutex)

## 类原型

```CPP
class recursive_mutex;
```

定义在头文件`<mutex>`中。

## 描述

`recursive_mutex`类是一个同步原语，可以被用在保护共享资源防止竞争。

`recursive_mutex`类提供一个独占，递归的所有权语义：

* 当一个调用线程成功调用`lock`或`try_lock`后，它获得`mutex`的所有权，这个线程运行多次调用