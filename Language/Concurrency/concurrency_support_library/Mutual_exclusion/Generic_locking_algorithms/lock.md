# lock

参考文档

* [lock](https://en.cppreference.com/w/cpp/thread/lock)

定义在`mutex`

## 函数原型

```CPP
template< class Lockable1, class Lockable2, class... LockableN >
void lock( Lockable1& lock1, Lockable2& lock2, LockableN&... lockn );
```

## 描述

锁定每个提供的`mutex`，为它们分别调用`lock`，使用防止死锁的算法。

`mutex`会被锁定，但是具体调用`lock`,`try_lock`,`unlock`的顺序未指定。如果其中一个互斥锁的`lock`抛出异常，则为已经锁定的互斥锁调用`unlock`。
