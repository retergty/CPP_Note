# once_flag

参考文档

* [once_flag](https://en.cppreference.com/w/cpp/thread/once_flag)

定义在`<mutex>`.

## 类原型

```CPP
class once_flag;
```

## 描述

一个用于`std::call_once`的帮助类。

将同一个`once_flag`作为不同的`std::call_once`函数的参数，可以保证`call_once`中的可调用对象只调用一次。

`once_flag`不可移动与复制。
