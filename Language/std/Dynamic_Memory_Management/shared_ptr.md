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

`shared_ptr`可以在拥有一个对象的所有权的同时，存储指向另外一个对象的指针。这个特性叫做别名(aliasing),可以用于指向成员对象的同时，拥有整个对象的所有权。存储的指针就是使用`get()`,解引用运算符，比较运算符使用的指针，而拥有所有权的指针就是指向被销毁对象的指针。

当`shared_ptr`不拥有任何对象的所有权时，它就被认为是空的，此时存储指针不一定为空。

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

  注意别名构造函数(aliasing constructor).

  ```CPP
  template< class Y >
  shared_ptr( const shared_ptr<Y>& r, element_type* ptr ) noexcept;	
  template< class Y >
  shared_ptr( shared_ptr<Y>&& r, element_type* ptr ) noexcept;
  ```

  通常用于保留管理对象，同时访问其中一部分。

  ```CPP
  struct Bar { 
      // some data that we want to point to
  };

  struct Foo {
      Bar bar;
  };

  shared_ptr<Foo> f = make_shared<Foo>(some, args, here);
  shared_ptr<Bar> specific_data(f, &f->bar);

  // ref count of the object pointed to by f is 2
  f.reset();

  // the Foo still exists (ref cnt == 1)
  // so our Bar pointer is still valid, and we can use it for stuff
  some_func_that_takes_bar(specific_data);
  ```

  这个特性和以下相同

  ```CPP
  Bar const& specific_data = Foo(...).bar;
  Bar&& also_specific_data = Foo(...).bar;
  ```

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
  
### 获取信息

* [get](https://en.cppreference.com/w/cpp/memory/shared_ptr/get)

  ```CPP
  element_type* get() const noexcept;
  ```

  返回存储指针。

* [operator*，operator->](https://en.cppreference.com/w/cpp/memory/shared_ptr/operator*)

  ```CPP
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  ```

  解引用存储指针。

* [operator[]](https://en.cppreference.com/w/cpp/memory/shared_ptr/operator_at)

  ```CPP
  element_type& operator[]( std::ptrdiff_t idx ) const;
  ```

  使用`index`获得存储指针数组的元素，只有在`T`为数组时有意义。

* [use_count](https://en.cppreference.com/w/cpp/memory/shared_ptr/use_count)

  ```CPP
  long use_count() const noexcept;
  ```

  返回使用计数，如果`*this`没有管理对象，返回`0`.

  在多线程环境下，这个返回值是估计值，因为时刻都有可能别的线程的`shared_ptr`析构。

  通常的用法如下

  * 函数返回值与`0`比较，如果为真，则`shared_ptr`为空，不管理任何对象（但是不保证存储指针是`nullptr`).
  * 函数返回值与`1`比较，如果为真，意味着目前没有别的拥有管理对象的`shared_ptr`.但在多线程环境下，这不意味着可以安全地修改管理对象，因为之前通过别的`shared_ptr`访问的管理对象的行动可能没有完成，或者是新的`shared_ptr`可能随时创建。

* [operator bool](https://en.cppreference.com/w/cpp/memory/shared_ptr/operator_bool)

  ```CPP
  explicit operator bool() const noexcept;
  ```

  检查是否存储指针为是`nullptr`.

* [owner_before](https://en.cppreference.com/w/cpp/memory/shared_ptr/owner_before)

  ```CPP
  template< class Y >
  bool owner_before( const shared_ptr<Y>& other ) const noexcept;
  template< class Y >
  bool owner_before( const std::weak_ptr<Y>& other ) const noexcept;
  ```

  检查是否`*this`管理指针是否小于`other`管理对象指针，如果`*this`先于`other`拥有管理对象，函数返回`true`。否则，返回`false`.

  如果两个`shared_ptr`都为空，则函数返回`true`.

  这是用在判断两个`shared_ptr`是否管理同一个管理对象或者都为空的相等判断中的。

  两个智能指针被认为是相等的，当且仅当它们共享同一个对象的所有权或者是均为空。与存储指针的值无关。

  ```CPP
  bool equivalent(p1, p2) {
    return !p1.owner_before(p2) && !p2.owner_before(p1);
  }
  ```
