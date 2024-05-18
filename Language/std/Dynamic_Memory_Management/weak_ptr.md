# weak_ptr

参考文档

* CPP Reference[weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)

定义在头文件`<memory>`中，是动态内存管理常用到的类对象。

## 类原型

```CPP
template< class T > class weak_ptr;
```

## 描述

`std::weak_ptr`是一个智能指针，行为和指针十分类似，保存着一个指向一个对象的引用，但是没有这个对象的所有权，这个对象的所有权是通过`std::shared_ptr`管理的。所以，当要访问`weak_ptr`存储的对象时，必须先转换为`shared_ptr`获取所有权之后，才能访问。

`weak_ptr`描述了暂时性的所有权模型，需要在一个对象存在时访问它，同时，这个对象可能会在任何时刻被别的程序流析构，`weak_ptr`用于跟随这个对象，并在适时转换为`shared_ptr`来短暂地获取所有权(同时可能会延长对象存在时间)。

另一个用法是打破引用环，如果两个`shared_ptr`形成了引用环，使用计数永远不可能为零，带来了内存泄漏，可以定义其中一个为`weak_ptr`来打破环。

## 成员类型定义

* `element_type`就是`std::remove_extent_t<T>`.

## 成员函数

### 构建对象

* [weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr)

* [operator=](https://en.cppreference.com/w/cpp/memory/weak_ptr/operator%3D)

  ```CPP
  weak_ptr& operator=( const weak_ptr& r ) noexcept;
  template< class Y >
  weak_ptr& operator=( const weak_ptr<Y>& r ) noexcept;
  template< class Y >
  weak_ptr& operator=( const shared_ptr<Y>& r ) noexcept;
  weak_ptr& operator=( weak_ptr&& r ) noexcept;
  template< class Y >
  weak_ptr& operator=( weak_ptr<Y>&& r ) noexcept;
  ```

  替换管理对象为`r`指向地管理对象。

### 修改weak_ptr

* [reset](https://en.cppreference.com/w/cpp/memory/weak_ptr/reset)

  ```CPP
  void reset() noexcept;
  ```

  释放所管理的对象，调用完毕后,`*this`不再管理任何对象。

* [swap](https://en.cppreference.com/w/cpp/memory/weak_ptr/swap)

  ```CPP
  void swap( weak_ptr& r ) noexcept;
  ```

  交换两个`weak_ptr`所管理的对象，这两个`weak_ptr`都可以为空。

### 获取信息

* [use_conut](https://en.cppreference.com/w/cpp/memory/weak_ptr/use_count)

  ```CPP
  long use_count() const noexcept;
  ```

  返回有多少个`shared_ptr`正在管理着对象。如果为`0`，则意味着`weak_ptr`所引用的对象已经被销毁了。`*this`为空。

* [expired](https://en.cppreference.com/w/cpp/memory/weak_ptr/expired)

  ```CPP
  bool expired() const noexcept;
  ```

  检查`weak_ptr`所引用的对象是否已经被删除了，等价于`use_count()==0`.

* [lock](https://en.cppreference.com/w/cpp/memory/weak_ptr/lock)

  ```CPP
  std::shared_ptr<T> lock() const noexcept;
  ```

  创建一个`shared_ptr`,获取`weak_ptr`所指向对象的所有权。如果`weak_ptr`为空，那么生成的`shared_ptr`也是空的。

  类似于`expired() ? shared_ptr<T>() : shared_ptr<T>(*this)`，但是保证操作的原子性。

  也可以使用`shared_ptr`的构造函数获得所有权。

* [owner_before](https://en.cppreference.com/w/cpp/memory/weak_ptr/owner_before)

  ```CPP
  template< class Y >
  bool owner_before( const weak_ptr<Y>& other ) const noexcept;
  template< class Y >
  bool owner_before( const std::shared_ptr<Y>& other ) const noexcept;
  ```

  检查是否`*this`管理指针是否小于`other`管理对象指针，如果`*this`先于`other`拥有管理对象，函数返回`true`。否则，返回`false`.
