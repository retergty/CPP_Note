# try_lock

参考文档

* [try_lock](https://en.cppreference.com/w/cpp/thread/try_lock)

定义在`mutex`

## 函数原型

```CPP
template< class Lockable1, class Lockable2, class... LockableN >
int try_lock( Lockable1& lock1, Lockable2& lock2, LockableN&... lockn );
```

## 描述

尝试锁定每个提供的`mutex`，为它们分别调用`try_lock`，顺序是从`lock1`到`lockn`.

如果其中一个互斥锁的`try_lock`失败，则不会继续`try_lock`其余的互斥锁,同时为已经锁定的互斥锁调用`unlock`,同时返回一个从`0`开始的序号，表示`try_lock`失败的互斥锁。

如果其中一个互斥锁的`try_lock`抛出异常，则为已经锁定的互斥锁调用`unlock`。

函数成功返回`-1`，否则返回一个从`0`开始的序号，表示`try_lock`失败的互斥锁。
