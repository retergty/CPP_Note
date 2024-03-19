# vector

参考文档

* CPP REFERENCE[std::vector](https://en.cppreference.com/w/cpp/container/vector)

定义在头文件`<vector>`.

## 类原型

```CPP
template<
    class T,
    class Allocator = std::allocator<T>
> class vector;    // (1)
namespace pmr {
    template< class T >
    using vector = std::vector<T, std::pmr::polymorphic_allocator<T>>;
}      // (2)
```

## 描述

`vector`的元素在内存区域连续存储，可以直接使用指针加上`offset`访问。

`vector`的容量是自动处理的，并在需要时候扩容，每次扩容为原来的两倍。使用`capacity()`返回已经分配给`vector`的内存总量。额外的内存可以通过`shrink_to_fit()`返回给系统。

重新分配`vector`的容量是性能高昂的操作，如果预先知道了元素的数量，那么可以使用`reserve()`保留给定数量的空间。

`vector`常见操作的复杂度

* 随机访问——常数 O(1)
* 在末尾插入或移除元素——均摊常数 O(1)
* 插入或移除元素到`vector`结尾的距离成线性 O(n)

## vector的迭代器

`vector`的迭代器类型是`Random Access Iterator`.

### 会使得迭代器失效的操作

* 所有的读取操作都不会失效`vector`的迭代器。
* `swap`,`std::swap`会失效`end()`迭代器。
* `clear`,`operator=`,`assign`总是会失效迭代器。
* `reserve`,`shrink_to_fit`如果改变了容量，那么失效所有迭代器，否则，不失效。
* `erase`会失效擦除的元素以及所有后面的迭代器,包括`end()`。
* `push_back`,`emplace_back`如果改变了容量，那么失效所有迭代器，否则，只失效`end()`。
* `insert`,`emplace`如果改变了容量，那么失效所有迭代器，否则，失效在插入点及之后的迭代器，包括`end()`.
* `resize`如果该变量容量，那么失效所有迭代器，否则，只有`end()`与任何擦除的元素的迭代器。
* `pop_back`擦除的元素以及`end()`的迭代器。

注意这些函数指的都是`vector`本身的操作，而不是元素的操作，比如成员函数`swap`的原型为

```CPP
void swap( vector& other );
```

表示的是两个`vector`互相交换。

## 常用成员函数

### 构建容器

* [vector](https://en.cppreference.com/w/cpp/container/vector/vector)

* [assign](https://en.cppreference.com/w/cpp/container/vector/assign)

  替换容器内的内容。

  ```CPP
  void assign( size_type count, const T& value );
  template< class InputIt >

  void assign( InputIt first, InputIt last );

  void assign( std::initializer_list<T> ilist );
  ```

  会失效所有迭代器。

### 访问数据

* [at](https://en.cppreference.com/w/cpp/container/vector/at)

  通过边界检查访问指定元素。

  ```CPP
  reference at( size_type pos );
  const_reference at( size_type pos ) const;
  ```

  如果`pos`不在容器的范围内，抛出异常。

* [data](https://en.cppreference.com/w/cpp/container/vector/data)

  直接访问底层的连续存储空间。

  ```CPP
  T* data() noexcept;
  const T* data() const;
  ```

  返回指向底层存储空间首元素的指针，指针合法范围为`[data(), data() + size())`

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/vector/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/vector/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。

* [reserve](https://en.cppreference.com/w/cpp/container/vector/reserve)

  ```CPP
  void reserve( size_type new_cap );
  ```

  提升`vector`的容量，使得容量满足或者多于`new_cap`,不会降低`vector`的容量。

  如果改变了容量，失效所有的迭代器。

* [capacity](https://en.cppreference.com/w/cpp/container/vector/capacity)

  ```CPP
  size_type capacity() const noexcept;
  ```

  返回`vector`目前的容量。

* [shrink_to_fit](https://en.cppreference.com/w/cpp/container/vector/shrink_to_fit)

  ```CPP
  void shrink_to_fit();
  ```

  移去没有使用的容量，会缩小`vector`的容量.

  如果改变了容量，失效所有的迭代器。

### 修改容器

* [clear](https://en.cppreference.com/w/cpp/container/vector/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除`vector`中所有的元素，调用完毕后，`size()`为`0`.

  失效所有的迭代器。

* [insert](https://en.cppreference.com/w/cpp/container/vector/insert)

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

  如果插入后，元素数目大于容量，会引发重分配，失效所有的迭代器。

  如果没有引发重分配，失效所有指向插入点后（包括插入点自身）的迭代器。

  这个函数是通过调用元素的复制构造函数或者是移动构造函数实现的。

* [emplace](https://en.cppreference.com/w/cpp/container/vector/emplace)

  ```CPP
  template< class... Args >
  iterator emplace( const_iterator pos, Args&&... args );
  ```

  在`pos`前原地构建新的元素。`args`就是传递给元素构造函数的参数。依照参数的不同，可能会调用复制构造或者是移动构造或者是特定的构造函数。

  元素会通过`std::allocator_traits::construct`函数直接就地构建新的元素。但是，如果指定的位置已经存在了元素，那么将要插入的元素会在别的地方先行构建并移动到要求的地方。

  如果插入后，元素数目大于容量，会引发重分配，失效所有的迭代器。

  如果没有引发重分配，失效所有指向插入点后（包括插入点自身）的迭代器。

* [erase](https://en.cppreference.com/w/cpp/container/vector/erase)

  ```CPP
  iterator erase( const_iterator pos );
  iterator erase( const_iterator first, const_iterator last );
  ```

  `1`擦除`pos`指向的元素。

  `2`擦除`[first,last)`范围指向的元素。

  显然`pos`不能是尾后迭代器。

  在插入点及其之后的迭代器会失效。

  返回指向删除元素之后的迭代器。

* [push_back](https://en.cppreference.com/w/cpp/container/vector/push_back)

  ```CPP
  void push_back( const T& value );
  void push_back( T&& value );
  ```

  将`value`添加到到`vector`末尾，具有常数复杂度。也就是，插入点为`end()`.

  如果插入后，元素数目大于容量，会引发重分配，失效所有的迭代器。

  如果没有引发重分配，失效所有指向插入点后（包括插入点自身）的迭代器。  

* [emplace_back](https://en.cppreference.com/w/cpp/container/vector/emplace_back)

  ```CPP
  template< class... Args >
  reference emplace_back( Args&&... args );
  ```

  在`vector`的末尾就地构建新的元素。

  返回指向构造的元素的引用。

  如果插入后，元素数目大于容量，会引发重分配，失效所有的迭代器。

  如果没有引发重分配，失效所有指向插入点后（包括插入点自身）的迭代器。  

* [pop_back](https://en.cppreference.com/w/cpp/container/vector/pop_back)

  ```CPP
  void pop_back();
  ```

  删除`vector`最后的一个元素。

  失效指向最后一个元素与`end()`的迭代器。

* [resize](https://en.cppreference.com/w/cpp/container/vector/resize)

  ```CPP
  void resize( size_type count );
  void resize( size_type count, const value_type& value );
  ```

  调整`vector`,使之包含`count`个元素。

  如果当前的元素数目多于`count`,将所有多余的元素删除。

  如果当前的元素数目少于`count`,`1`会插入默认值，`2`会插入指定值。

  注意，这个函数不会修改容量。

* [swap](https://en.cppreference.com/w/cpp/container/vector/swap)

  函数原型为

  ```CPP
  void swap( vector& other ) noexcept;
  ```

  交换两个`vector`的内容与容量，**不会调用**任何容器内元素`move`,`copy`,`swap`操作。

  除了`end()`迭代器失效，任何其他的迭代器与引用均不失效，指向原来的位置。（但是所属的`vector`不同了）。

  ```CPP
  std::vector<int> a1{1, 2, 3}, a2{4, 5};
  
  auto it1 = std::next(a1.begin());
  auto it2 = std::next(a2.begin());

  int& ref1 = a1.front();
  int& ref2 = a2.front();

  std::cout << a1 << a2 << *it1 << ' ' << *it2 << ' ' << ref1 << ' ' << ref2 << '\n';
  a1.swap(a2);
  std::cout << a1 << a2 << *it1 << ' ' << *it2 << ' ' << ref1 << ' ' << ref2 << '\n';
  ```

  输出

  ```text
  { 1 2 3 } { 4 5 } 2 5 1 4
  { 4 5 } { 1 2 3 } 2 5 1 4
  ```

### 常用非成员函数

## 比较两个`vector`的运算符重载函数
