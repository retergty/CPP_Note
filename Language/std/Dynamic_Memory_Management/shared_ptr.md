# shared_ptr

参考文档

* CPP Reference[shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)

定义在头文件`<memory>`中，是动态内存管理常用到的类对象。

## 类原型

```CPP
template< class T > class shared_ptr;
```

## 描述

`std::shared_ptr`是一个行为和指针十分类似的类，它通过指针取得动态分配的对象的所有权。多个`shared_ptr`可以指向同一个对象。

由`shared_ptr`所拥有的对象当如下情况满足时被销毁，并释放其所占有的动态内存。

* 最后一个拥有这个对象的`shared_ptr`被销毁
* 最后一个拥有这个对象的`shared_ptr`被分配给另一个指针通过函数`operator=`或`reset()`.

动态分配的对象是使用`delete`或者是用户自定义的删除器销毁的。

`shared_ptr`可以在拥有一个对象的所有权的同时，存储指向另外一个对象的指针。这个特性可以用于指向成员对象的同时，拥有整个对象的所有权。存储的指针就是使用`get()`,解引用运算符，比较运算符使用的指针，而拥有所有权的指针就是指向被销毁对象的指针。

当`shared_ptr`不拥有任何对象的所有权时，它就被认为是空的。

所有的`shared_ptr`都是`复制可构造(CopyConstructible)`，`复制可赋值(CopyAssignable)`，以及可`小于比较(LessThanComparable)`的。

`shared_ptr`**所有的**成员函数，都可以在不同线程的不同的`shared_ptr`对象上调用，不需要额外的同步操作，哪怕这些`shared_ptr`对象是一个`shared_ptr`的复制，且拥有同一个对象的所有权。但是，在不同线程操作同一个`shared_ptr`对象会带来竞争，`std::atomic<shared_ptr>`可以避免这个事情发生。

## 概念

### 管理对象managed object

管理对象就是`shared_ptr`所管理的动态分配对象，当最后一个拥有管理对象的`shared_ptr`析构时，随之析构管理对象。

### 存储指针stored pointer

存储指针就是`shared_ptr`在函数`get()`,解引用运算符，比较运算符中所使用的指针，可以不指向管理对象。

### 管理指针managed pointer

管理指针就是指向管理对象的指针，析构管理对象时使用删除器通过管理指针删除管理对象。

### 使用计数use_count

记录目前有多少个`shared_ptr`正在管理这个管理对象。每当新增一个`shared_ptr`管理这个对象时，使用计数都会加一，每当一个管理这个对象的`shared_ptr`不再管理这个对象（析构或者是`reset`）后,使用计数都会减一。

## 成员类型定义

* `element_type`就是`std::remove_extent_t<T>`.
* `weak_type`就是`std::weak_ptr<T>`

## 成员函数

### 构建对象

* [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr)

  可以从指针构建`shared_ptr`,也可以指定用户定义的删除器，接管之前指针管理的动态内存对象。

* [operator=](https://en.cppreference.com/w/cpp/memory/shared_ptr/operator%3D)

  ```CPP
  shared_ptr& operator=( const shared_ptr& r ) noexcept;
  template< class Y >
  shared_ptr& operator=( const shared_ptr<Y>& r ) noexcept;
  shared_ptr& operator=( shared_ptr&& r ) noexcept;
  template< class Y >
  shared_ptr& operator=( shared_ptr<Y>&& r ) noexcept;
  template< class Y >
  template< class Y, class Deleter >
  shared_ptr& operator=( std::unique_ptr<Y, Deleter>&& r );
  ```

  把管理对象更换为`r`所管理的对象。如果`r`是空的，则`*this`也会置为空。

  如果`*this`已经管理了一个对象，且是最后一个还拥有这个对象管理权，且`r`和`*this`不相同，则管理对象被析构。

  左值版本的`operator=`复制了`r`管理对象的的所有权，右值版本的`operator=`转移了`r`管理对象的所有权。

### 修改shared_ptr

* [reset](https://en.cppreference.com/w/cpp/memory/shared_ptr/reset)

  ```CPP
  void reset() noexcept;
  template< class Y >
  void reset( Y* ptr );
  template< class Y, class Deleter >
  void reset( Y* ptr, Deleter d );
  template< class Y, class Deleter, class Alloc >
  void reset( Y* ptr, Deleter d, Alloc alloc );
  ```

  把管理对象更换为`ptr`指向的对象。可以提供用户定义的删除器。

  如果`*this`已经管理了一个对象，且是最后一个还拥有这个对象的管理权，则使用`*this`存储的删除器析构管理对象。

  `Y`必须可以转换为`T`,且是完整类型。

* [swap](https://en.cppreference.com/w/cpp/memory/shared_ptr/swap)

  ```CPP
  void swap( shared_ptr& r ) noexcept;
  ```

  交换两个`shared_ptr`所管理的对象，这两个`shared_ptr`都可以为空。
  