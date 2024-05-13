# queue

参考文档

* CPP REFERENCE[std::queue](https://en.cppreference.com/w/cpp/container/queue)

定义在头文件`<queue>`.

## 类原型

```CPP
template<
    class T,
    class Container = std::deque<T>
> class queue;
```

## 描述

`queue`是一个容器适配器，给用户提供了队列的数据结构，先进先出(FIFO).

`queue`作为一个包装器，包装了底层容器，只提供了一系列特别的函数访问底层数据结构。比如队列从底层容器末端压入或者从首端弹出元素。

底层数据结构默认是`deque`.

模版参数`Container`必须满足顺序容器的要求，此外，还需要提供`back()`,`front()`,`push_back()`,`pop_front()`函数。标准库中，`std::deque`,`std::list`满足这些要求，都可以作为模版参数的实参。

## deque的迭代器

`deque`不提供迭代器，不需要在`deque`中进行迭代。

## 常用成员函数

### 构建容器

* [queue](https://en.cppreference.com/w/cpp/container/stack/stack)

* [operator=](https://en.cppreference.com/w/cpp/container/stack/operator%3D)

### 访问元素

* [front](https://en.cppreference.com/w/cpp/container/queue/front)

  ```CPP
    reference front();
    const_reference front() const;

  ```

  返回指向队列第一个元素的引用，也就是会在下一次`pop`中弹出的元素。

* [back](https://en.cppreference.com/w/cpp/container/queue/back)

  ```CPP
    reference front();
    const_reference front() const;

  ```

  返回指向队列最后一个元素的引用，也就是会在最近一次`push`中压入的元素。

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/queue/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  检测底层容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/queue/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回底层容器当前存储的元素个数。

### 修改容器

* [push](https://en.cppreference.com/w/cpp/container/queue/push)

  ```CPP
  void push( const value_type& value );
  void push( value_type&& value );
  ```

  将指定元素添加到队列的末尾。

* [emplace](https://en.cppreference.com/w/cpp/container/queue/emplace)

  ```CPP
  template< class... Args >
  decltype(auto) emplace( Args&&... args );
  ```
  
  使用参数`args`在队列末尾构建元素，通常只是调用`c.emplace_back(std::forward<Args>(args)...)`

  返回指向压入元素的引用。

* [pop](https://en.cppreference.com/w/cpp/container/queue/pop)

  ```CPP
  void pop();
  ```

  将队列前端的第一个元素弹出。

* [swap](https://en.cppreference.com/w/cpp/container/queue/swap)

  ```CPP
  void swap( stack& other ) noexcept;
  ```

  交换两个队列的元素。
