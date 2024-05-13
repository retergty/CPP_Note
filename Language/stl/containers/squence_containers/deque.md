# deque

参考文档

* CPP REFERENCE[std::deque](https://en.cppreference.com/w/cpp/container/deque)

定义在头文件`<deque>`.

## 类原型

```CPP
template<
    class T,
    class Allocator = std::allocator<T>
> class deque;
namespace pmr {
    template< class T >
    using deque = std::deque<T, std::pmr::polymorphic_allocator<T>>;
}
```

## 描述

`deque`是一个双端的队列，允许快速在首尾插入和删除元素，同时保持快速的随机访问能力（性能劣于`vector`。同样地在首尾插入或删除元素不会失效除了尾后迭代器的所有迭代器。

`deque`的内部结构是一个个区块，第一个区块朝一个方向拓展，另一个区块朝另一个方向拓展，并用一个映射结构跟踪这些块。这样保持了高性能的随机访问。不同于`vector`，它也不需要重分配空间。

## 迭代器类型

迭代器类型是`Random Access Iterator`.

### 会使得迭代器失效的操作

* 插入，添加端点的元素不会失效指向其它元素的引用，但是会**失效**所有迭代器。
* 删除端点的元素不会失效指向其它元素的迭代器，删除末尾的元素会失效尾后迭代器。
* 删除其它位置的元素会失效所有迭代器。
* `resize`减小容量时会失效指向删除元素的迭代器和尾后迭代器，增大容量时不会失效任何迭代器。

## 常用成员函数

### 构建容器

* [deque](https://en.cppreference.com/w/cpp/container/deque/deque)

* [assign](https://en.cppreference.com/w/cpp/container/deque/assign)

  替换容器内的数据。

  ```CPP
  void assign( size_type count, const T& value );
  template< class InputIt >
  void assign( InputIt first, InputIt last );
  void assign( std::initializer_list<T> ilist );
  ```

  会失效所有的迭代器。

### 访问数据

* [at](https://en.cppreference.com/w/cpp/container/deque/at)

  通过边界检查访问指定元素。

  ```CPP
  reference at( size_type pos );
  const_reference at( size_type pos ) const;
  ```

  如果`pos`不在容器的范围内，抛出异常。

* [operator[]](https://en.cppreference.com/w/cpp/container/deque/operator_at)

  访问指定元素。

  ```CPP
  reference operator[]( size_type pos );
  const_reference operator[]( size_type pos ) const;
  ```

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/deque/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/deque/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。

* [shrink_to_fit](https://en.cppreference.com/w/cpp/container/deque/shrink_to_fit)

  ```CPP
  void shrink_to_fit();
  ```

  移去没有使用的容量，会缩小`deque`的容量.

  会失效所有的迭代器。

### 修改容器

* [clear](https://en.cppreference.com/w/cpp/container/deque/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除`deque`中所有的元素，调用完毕后，`size()`为`0`.

  失效所有的迭代器。

* [insert](https://en.cppreference.com/w/cpp/container/deque/insert)

  ```CPP
  iterator insert( const_iterator pos, const T& value );
  iterator insert( const_iterator pos, T&& value );
  iterator insert( const_iterator pos, size_type count, const T& value );
  template< class InputIt >
  iterator insert( const_iterator pos, InputIt first, InputIt last );
  iterator insert( const_iterator pos, std::initializer_list<T> ilist );
  ```

  `1`和`2`将`value`插入到`pos`前面。

  `3`将`count`个`value`的副本插入到`pos`前面。

  `4`利用输入迭代器，把`[first,last)`范围的元素插入到`pos`前。
  
  `5`将初始化列表`ilist`的元素插入到`pos`前面。

  `1`和`2`返回指向`value`的迭代器。

  `3`,`4`,`5`返回指向第一个插入的元素的迭代器。

  如果插入点是两端，不会失效引用。

  无论插入位置，都会失效所有的迭代器。

* [emplace](https://en.cppreference.com/w/cpp/container/deque/emplace)

  ```CPP
  template< class... Args >
  iterator emplace( const_iterator pos, Args&&... args );
  ```

  在`pos`前原地构建新的元素。`args`就是传递给元素构造函数的参数。依照参数的不同，可能会调用复制构造或者是移动构造或者是特定的构造函数。

  元素会通过`std::allocator_traits::construct`函数直接就地构建新的元素。但是，如果指定的位置已经存在了元素，那么将要插入的元素会在别的地方先行构建并移动到要求的地方。

  失效所有的迭代器。

  如果插入点是两端，不会失效引用。

* [erase](https://en.cppreference.com/w/cpp/container/deque/erase)

  ```CPP
  iterator erase( const_iterator pos );
  iterator erase( const_iterator first, const_iterator last );
  ```

  `1`擦除`pos`指向的元素。

  `2`擦除`[first,last)`范围指向的元素。

  显然`pos`不能是尾后迭代器。

  如果删除两端的元素，指向其他元素的迭代器不会失效。删除尾端的元素还会失效尾后迭代器。如果删除其他位置的元素，所有迭代器均失效。

* [push_back](https://en.cppreference.com/w/cpp/container/deque/push_back)

  ```CPP
  void push_back( const T& value );
  void push_back( T&& value );
  ```

  将`value`添加到到`deque`末尾，具有常数复杂度。也就是，插入点为`end()`.

  会失效所有的迭代器。不会失效任何引用。

* [emplace_back](https://en.cppreference.com/w/cpp/container/deque/emplace_back)

  ```CPP
  template< class... Args >
  reference emplace_back( Args&&... args );
  ```

  在`deque`的末尾就地构建新的元素。

  返回指向构造的元素的引用。

  会失效所有的迭代器。不会失效任何引用。

* [pop_back](https://en.cppreference.com/w/cpp/container/deque/pop_back)

  ```CPP
  void pop_back();
  ```

  删除`deque`最后一个元素。

  如果`deque`为空，则行为未定义。

  失效指向最后一个元素与`end()`的迭代器。

* [push_front](https://en.cppreference.com/w/cpp/container/deque/push_front)

  ```CPP
  void push_front( const T& value );
  void push_front( T&& value );
  ```

  将`value`添加到到`deque`头部.

  会失效所有的迭代器。不会失效任何引用。

* [emplace_front](https://en.cppreference.com/w/cpp/container/deque/emplace_front)

  ```CPP
  template< class... Args >
  reference emplace_front( Args&&... args );
  ```

  在`deque`的头部就地构建新的元素。

  返回指向构造的元素的引用。

  会失效所有的迭代器。不会失效任何引用。

* [pop_front](https://en.cppreference.com/w/cpp/container/deque/pop_front)

  ```CPP
  void pop_front();
  ```

  删除`deque`第一个元素。

  失效指向第一个元素的迭代器。

* [resize](https://en.cppreference.com/w/cpp/container/deque/resize)

  ```CPP
  void resize( size_type count );
  void resize( size_type count, const value_type& value );
  ```

  调整`vector`,使之包含`count`个元素。

  如果当前的元素数目多于`count`,将所有多余的元素删除。

  如果当前的元素数目少于`count`,`1`会插入默认值，`2`会插入指定值。
