# stack

参考文档

* CPP REFERENCE[std::stack](https://en.cppreference.com/w/cpp/container/stack)

定义在头文件`<stack>`.

## 类原型

```CPP
template<
    class T,
    class Container = std::deque<T>
> class stack;
```

## 描述

`stack`是一个容器适配器，给用户提供了栈的数据结构，先进后出（LIFO）.

`stack`作为一个包装器，包装了底层容器，只提供了一系列特别的函数访问底层数据结构。栈从底层容器末尾压入或者弹出元素。

底层数据结构默认是`deque`.

模版参数`Container`必须满足顺序容器的要求，此外，还需要提供`back()`,`push_back()`,`pop_back()`函数。标准库中，`std::vector`,`std::deque`,`std::list`都满足这些要求，都可以作为模版参数的实参。

## stack的迭代器

`stack`不提供迭代器，不需要在`stack`中进行迭代。

## 常用成员函数

### 构建容器

* [stack](https://en.cppreference.com/w/cpp/container/stack/stack)

* [operator=](https://en.cppreference.com/w/cpp/container/stack/operator%3D)

### 访问元素

* [top](https://en.cppreference.com/w/cpp/container/stack/top)

  ```CPP
  reference top();
  const_reference top() const;
  ```

  返回栈顶元素的引用。

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/deque/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  检测底层容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/deque/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回底层容器当前存储的元素个数。

### 修改容器

* [push](https://en.cppreference.com/w/cpp/container/stack/push)

  ```CPP
  void push( const value_type& value );
  void push( value_type&& value );
  ```

  将指定元素压入到栈顶。

* [emplace](https://en.cppreference.com/w/cpp/container/stack/emplace)

  ```CPP
  template< class... Args >
  decltype(auto) emplace( Args&&... args );
  ```
  
  使用参数`args`在栈顶原地构建元素，通常只是调用`c.emplace_back(std::forward<Args>(args)...)`

  返回指向压入元素的引用。

* [pop](https://en.cppreference.com/w/cpp/container/stack/pop)

  ```CPP
  void pop();
  ```

  将栈顶的元素推出。调用函数`c.pop_back()`

* [swap](https://en.cppreference.com/w/cpp/container/stack/swap)

  ```CPP
  void swap( stack& other ) noexcept;
  ```

  交换两个栈的元素。
