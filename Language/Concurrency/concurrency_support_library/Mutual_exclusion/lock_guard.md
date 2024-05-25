# lock_guard

参考文档

* [lock_guard](https://en.cppreference.com/w/cpp/thread/lock_guard)

## 类原型

```CPP
template< class Mutex >
class lock_guard;
```

`Mutex`是锁定的`mutex`的类型，必须符合`BasicLockable`要求。

## 描述

`lock_guard`是一个互斥锁的包装器，提供了`RAII`机制管理互斥锁。

当一个`lock_guard`类创建时，它就会尝试获取传递给构造函数的互斥锁的所有权。当控制流离开了`lock_guard`的作用域后，`lock_guard`析构并释放锁定的互斥锁。

`lock_guard`是不能复制的类。

## 成员函数

### 构造与析构

* [lock_guard](https://en.cppreference.com/w/cpp/thread/lock_guard/lock_guard)

  ```CPP
  explicit lock_guard( mutex_type& m );
  lock_guard( mutex_type& m, std::adopt_lock_t t );
  lock_guard( const lock_guard& ) = delete; 
  ```

  获得`m`的所有权。

  `1`调用`m.lock()`获取互斥锁的所有权。

  `2`获得`m`的所有权，不尝试锁定它。当前线程必须已经锁定了这个互斥锁，否则程序未定义。

  `3`禁止复制构造函数。

* [~lock_guard](https://en.cppreference.com/w/cpp/thread/lock_guard/%7Elock_guard)

  ```CPP
  ~lock_guard();
  ```

  释放互斥锁的所有权，调用`m.unlock()`.,`m`就是传递给`lock_guard`的构造函数的参数。

## 常见错误

有时会忘记给`lock_guard`一个名字，比如`std::lock_guard(mtx)`，默认构建一个名为`mtx`的`lock_guard`.或`std::lock_guard{mtx}`构建一个`prvalue`的`lock_guard`并立即析构，这些都是错误的。
