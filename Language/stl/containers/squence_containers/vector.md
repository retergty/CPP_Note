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

  返回容器是否为空。

  ```CPP
  bool empty() const noexcept;
  ```

* [size](https://en.cppreference.com/w/cpp/container/vector/size)

  返回容器当前存储的元素个数。

  ```CPP
  size_type size() const noexcept;
  ```

### 修改容器

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
