# priority_queue

参考文档

* CPP REFERENCE[std::priority_queue](https://en.cppreference.com/w/cpp/container/priority_queue)

定义在头文件`<priority_queue>`.

## 类原型

```CPP
template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
> class priority_queue;
```

## 描述

`priority_queue`是一个容器适配器，给用户提供了优先队列的数据结构，可以在常数时间内查找最大的元素，在对数时间内插入和删除元素。

`priority_queue`通常实现是最大堆。

默认是查找最大值，但是通过修改`Compare`也可以查找最小值。比如`std::greater<T>`.

使用优先队列就像是在随机访问容器上使用堆，但是还具有不会意外地失效堆的情况。

模版参数`Container`必须满足顺序容器，且它的迭代器必须是随机访问迭代器，此外，还需要提供`front()`,`push_back()`,`pop_back()`函数接口。标准库中，`std::vector`,`std::deque`都满足这个需求。

## priority_queue的迭代器

`priority_queue`不提供迭代器，不需要在`priority_queue`中进行迭代。

## 常用成员函数

### 构建容器

* [priority_queue](https://en.cppreference.com/w/cpp/container/priority_queue/priority_queue)

* [operator=](https://en.cppreference.com/w/cpp/container/priority_queue/operator%3D)

### 访问元素

* [top](https://en.cppreference.com/w/cpp/container/stack/top)

  ```CPP
  const_reference top() const;
  ```

  返回栈顶元素的引用。这个元素会在下一次的`pop`中被移除。默认就是这个队列中的最大元素。

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/priority_queue/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  检测底层容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/priority_queue/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回底层容器当前存储的元素个数。

### 修改容器

* [push](https://en.cppreference.com/w/cpp/container/priority_queue/push)

  ```CPP
  void push( const value_type& value );
  void push( value_type&& value );
  ```

  将指定元素加入到优先队列中，时间复杂度为对数时间。

* [emplace](https://en.cppreference.com/w/cpp/container/priority_queue/emplace)

  ```CPP
  template< class... Args >
  void emplace( Args&&... args );
  ```
  
  使用参数`args`在优先队列中原地构建元素

* [pop](https://en.cppreference.com/w/cpp/container/priority_queue/pop)

  ```CPP
  void pop();
  ```

  将优先队列中的队头元素推出，也就是最大的元素。调用函数`c.pop_back()`,时间复杂度为对数时间。

* [swap](https://en.cppreference.com/w/cpp/container/priority_queue/swap)

  ```CPP
  void swap( priority_queue& other ) noexcept;
  ```

  交换两个优先队列的元素。
