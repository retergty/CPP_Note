# unique_ptr

参考文档

* CPP Reference[unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)

定义在头文件`<memory>`中，是动态内存管理常用到的类对象。

## 类原型

```CPP
template<
    class T,
    class Deleter = std::default_delete<T>
> class unique_ptr;
template <
    class T,
    class Deleter
> class unique_ptr<T[], Deleter>;
```

## 描述

`unique_ptr`是一个智能指针，行为和指针十分类似，它通过指针取得一个的对象的所有权，当对象离开作用域时，它按照要求处置对象。

由`unique_ptr`所拥有的对象当如下情况满足时被处置，使用用户指定的`Deleter`，使用表达式`get_deleter()(get())`删除对象，

* 管理这个对象的`unique_ptr`被析构。
* 管理这个对象的`unique_ptr`使用函数`operator=`或`reset()`分配给了另一个对象。

一个`unique_ptr`可以不管理任何对象，此时，这个`unique_ptr`就被认为是空的。

主要有两种`unique_ptr`,第一种是管理一个对象的`unique_ptr`，比如使用`new`创建的对象；第二种是管理一个数组的`unique_ptr`,比如使用`new[]`创建的对象。

`unique_ptr`满足移动可构造(MoveConstructible)，移动可赋值(MoveConstructible),但不满足复制可构造(CopyConstructible)和复制可赋值(CopyAssignable).也就是说，复制构造和复制赋值函数是不可访问的，这也是`unique_ptr`和`shared_ptr`不同之处，`unique_ptr`不会与别的`unique_ptr`共享对象的所有权。

只有非`const`的`unique_ptr`可以转移所有权给另一个`unique_ptr`,(因为移动函数接受非`const`的右值引用)。所以，当一个对象被`const`的`unique_ptr`管理时，实际上它的生命周期会被限制在这个`unique_ptr`对象的作用域中。

`unique_ptr`常用情况如下

* 为动态分配的对象提供异常安全。
* 传递所有权
* 代替裸指针使用在容器中，比如将`unique_ptr`移动到容器中，当容器删除这个元素时，自动释放管理的元素的内存。

如果`T`是`B`的派生类，那么`std::unique_ptr<T>`可以被隐式转换为`std::unique_ptr<B>`,并且`std::unique_ptr<B>`删除器会使用`B`的删除器，如果析构函数不是虚的，程序未定义。注意，`std::shared_ptr`行为不一样，`std::shared_ptr<B>`总是会选择`T`的删除器，哪怕析构函数不是虚函数。

## 成员函数

### 构建对象

* [unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr/unique_ptr)

* [operator=](https://en.cppreference.com/w/cpp/memory/unique_ptr/operator%3D)

  可以把不同的删除器赋值给`unique_ptr`,但是这两个删除器必须可以转换。

### 修改unique_ptr

* [release](https://en.cppreference.com/w/cpp/memory/unique_ptr/release)

  释放所管理对象的所有权，如果有的话。不是销毁管理对象，而是`unique_ptr`不再管理这个对象的所有权了。

  函数调用后，`get()`函数返回`nullptr`.

  函数返回指向所管理对象的指针，或者是`nullptr`

* [reset](https://en.cppreference.com/w/cpp/memory/unique_ptr/reset)

  ```CPP
  void reset( pointer ptr = pointer() ) noexcept;
  ```

  替换当前管理的对象，同时删除这个对象。

  会做如下的事

  1. 保存当前指针，`old_ptr = current_ptr`.
  2. 替换当前指针为函数参数，`current_ptr = ptr`.
  3. 如果`old_ptr`不为空，调用删除器，`if (old_ptr) get_deleter()(old_ptr)`.

  如果想要提供新的删除器，可以使用`operator=`.不会检测是否是自`reset`.也就是`p.reset(p.get())`代码错误。

* [swap](https://en.cppreference.com/w/cpp/memory/unique_ptr/swap)

  ```CPP
  void swap( unique_ptr& other ) noexcept;
  ```

  交换两个`unique_ptr`所管理的对象。

### 获取信息

* [get](https://en.cppreference.com/w/cpp/memory/unique_ptr/get)

  ```CPP
  pointer get() const noexcept;
  ```

  返回存储指针，或者`nullptr`.

* [get_deleter](https://en.cppreference.com/w/cpp/memory/unique_ptr/get_deleter)

  ```CPP
  Deleter& get_deleter() noexcept;
  const Deleter& get_deleter() const noexcept;
  ```

  返回删除器对象。

* [operator bool](https://en.cppreference.com/w/cpp/memory/unique_ptr/operator_bool)

  ```CPP
  explicit operator bool() const noexcept;
  ```

  返回是否`*this`管理了一个对象。

### `unique_ptr<T>`特有的函数

* [operator*,operator->](https://en.cppreference.com/w/cpp/memory/unique_ptr/operator*)

### `unique_ptr<T[]>`特有的函数

* [operator[]](https://en.cppreference.com/w/cpp/memory/unique_ptr/operator_at)

## 非成员函数

* [make_unique](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)

  构建`T`类型的对象，并使用`unique_ptr`管理，为数组模板实参提供了一系列特定的函数。

  ```CPP
  template< class T, class... Args >
  unique_ptr<T> make_unique( Args&&... args );
  template< class T >
  unique_ptr<T> make_unique( std::size_t size );
  template< class T, class... Args >
  /* unspecified */ make_unique( Args&&... args ) = delete;
  ```

  不允许给有范围的模板实参创建。

  只有`T`是数组，后两个函数才会加入重载决议。

  例子如下

  ```CPP
  // Use the default constructor.
  std::unique_ptr<Vec3> v1 = std::make_unique<Vec3>();
  // Use the constructor that matches these arguments.
  std::unique_ptr<Vec3> v2 = std::make_unique<Vec3>(0, 1, 2);
  // Create a unique_ptr to an array of 5 elements.
  std::unique_ptr<Vec3[]> v3 = std::make_unique<Vec3[]>(5);
  ```
