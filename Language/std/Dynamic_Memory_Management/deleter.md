# deleter

在`unique_ptr`或`shared_ptr`要销毁对象时，它会调用`deleter`来**析构**所管理的对象并**释放**内存空间。注意和`allocator`区分，自定义`allocator`不需要我们手动析构所管理的对象，只需要我们释放分配的空间即可。

## 默认删除器

参考文档

* [default_delete](https://en.cppreference.com/w/cpp/memory/default_delete)

标准库提供了默认的删除器，定义在头文件`<memory>`

### 原型

```CPP
template< class T > struct default_delete;
template< class T > struct default_delete<T[]>;
```

非特化版本的`default_delete`使用`delete`析构并释放对象占用的内存空间。

特化版本的`default_delete`使用`delete[]`析构并释放对象数组所占用的内存空间。

### 成员函数

* [default_delete](https://en.cppreference.com/w/cpp/memory/default_delete)

  ```CPP
  constexpr default_delete() noexcept = default;

  Primary template specializations
  template< class U >
  default_delete( const default_delete<U>& d ) noexcept;

  Array specializations
  template< class U >
  default_delete( const default_delete<U[]>& d ) noexcept;
  ```

  `2`加入重载决议只有当`U*`可以隐式转换为`T*`,通常是派生类指针转换为基类指针。

  `3`加入重载决议只有当`U(*)[]`可以隐式转换为`T(*)[]`.

  这两个转换构造函数使得`std::unique_ptr<Derived>`隐式转换到`std::unique_ptr<Base>`成为可能。

* [operator()](https://en.cppreference.com/w/cpp/memory/default_delete)

  ```CPP
  Primary template specializations
  void operator()( T* ptr ) const;

  Array specializations
  template< class U >
  void operator()( U* ptr ) const;
  ```

  `1`在`ptr`处调用`delete`.

  `2`在`ptr`处调用`delete[]`.

## 用户定义删除器

参考文档

* [Why and when do I need to supply my own deleter?](https://stackoverflow.com/questions/51278175/why-and-when-do-i-need-to-supply-my-own-deleter)

智能指针不只是为了管理动态内存而设计出来的，它是为了实现`RAII(Resource Acquisition Is Initialization)`而提出的，我们当然可以使用智能指针管理文件指针，网络连接等。此时，我们便需要提供自己定义的删除器，

为了定义删除器，我们只需定义一个可调用对象，这个可调用对象可以接受`T*`类型的参数，并在这个函数里做析构对象和特定的销毁任务。

### 例子

在网络连接中，当控制流复杂时或异常发生时，可能没有释放网络资源。

```CPP
struct destination; // represents what we are connecting to
struct connection; // information needed to use the connection
connection connect(destination*); // open the connection
void disconnect(connection); // close the given connection
void f(destination &d /* other parameters */)
{
// get a connection; must remember to close it when done
connection c = connect(&d);
// use the connection
// if we forget to call disconnect before exiting f, there will be no way to closes
}
```

网络连接例子中，使用`shared_ptr`管理网络资源，让其自动释放。

```CPP
void end_connection(connection *p) { disconnect(*p); }
void f(destination &d /* other parameters */)
{
connection c = connect(&d);
shared_ptr<connection> p(&c, end_connection);
// use the connection
// when f exits, even if by an exception, the connection will be properly closed
}
```
