# scoped_lock

参考文档

* [scoped_lock](https://en.cppreference.com/w/cpp/thread/scoped_lock)

## 类原型

```CPP
template< class... MutexTypes >
class scoped_lock;
```

定义在头文件`<mutex>`中。

`Mutex`是锁定的`mutex`的类型，必须符合`Lockable`要求。

## 描述

`scoped_lock`是一个互斥锁包装器，提供了`RAII`风格的机制用于在`scoped_lock`生命周期内拥有零个或者多个互斥锁的所有权。

当一个`scoped_lock`被创建时，它就会尝试获取给定的互斥锁的所有权，当`scoped_lock`生命周期结束时，`scoped_lock`被销毁并释放之前锁定的互斥锁。如果给出了多个互斥锁，则会使用防止死锁的算法，与`std::lock`相同。

`scoped_lock`不可被复制。

## 成员类定义

* 如果只提供了一个`mutex`模版实参，则定义`mutex_type`为`Mutex`.

## 成员函数

* [scoped_lock](https://en.cppreference.com/w/cpp/thread/scoped_lock/scoped_lock)

  ```CPP
  explicit scoped_lock( MutexTypes&... m );
  scoped_lock( std::adopt_lock_t, MutexTypes&... m );
  scoped_lock( const scoped_lock& ) = delete;
  ```

  构造函数，获取给定`mutex`的所有权。

  `1`如果`sizeof...(MutexTypes) == 0`什么都不做，如果`sizeof...(MutexTypes) == 1`，调用`m.lock()`,其余情况下，调用`std::lock(m...)`.

  `2`，假定调用线程早已经锁定了这些互斥锁。

  `3`复制构造函数被删除。

* [~scoped_lock](https://en.cppreference.com/w/cpp/thread/scoped_lock/~scoped_lock)

  ```CPP
  ~scoped_lock();
  ```

  销毁`scoped_lock`,同时释放所关联的互斥锁的所有权，为每个互斥锁调用`unlock()`.
