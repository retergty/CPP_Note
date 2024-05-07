# forward_list

参考文档

* CPP REFERENCE[std::forward_list](https://en.cppreference.com/w/cpp/container/forward_list)

定义在头文件`<forward_list>`.

## 类原型

```CPP
template<
    class T,
    class Allocator = std::allocator<T>
> class forward_list;
namespace pmr {
    template< class T >
    using forward_list = std::forward_list<T, std::pmr::polymorphic_allocator<T>>;
}
```

## 描述

`forward_list`是一个支持快速插入与删除元素，但不支持快速随机访问与双向访问的容器，它的实现是单向列表。与`list`相比，这个容器更省空间，但却不支持双向访问。

## 迭代器的类型

迭代器的类型为传统前向迭代器`LegacyForwardIterator`.

### 会使得迭代器失效的操作

添加，删除，移动元素不会失效指向别的元素的迭代器。

添加，移动元素不会失效指向该元素的迭代器，但是删除操作会失效指向该元素的迭代器。

## 常用成员函数

### 构建容器

* [forward_list](https://en.cppreference.com/w/cpp/container/forward_list/forward_list)

* [operator=](https://en.cppreference.com/w/cpp/container/forward_list/operator%3D)

* [assign](https://en.cppreference.com/w/cpp/container/forward_list/assign)

  ```CPP
  void assign( size_type count, const T& value );
  template< class InputIt >
  void assign( InputIt first, InputIt last );
  void assign( std::initializer_list<T> ilist );
  ```

  替换容器的内容。

### 成员访问

* [front](https://en.cppreference.com/w/cpp/container/forward_list/front)

  ```CPP
  reference front();
  const_reference front() const;
  ```

  返回容器第一个元素的引用。

  如果容器为空，操作是未定义的。

### 迭代器

* [before_begin, cbefore_begin](https://en.cppreference.com/w/cpp/container/forward_list/before_begin)

  ```CPP
  iterator before_begin() noexcept;
  const_iterator before_begin() const noexcept;
  const_iterator cbefore_begin() const noexcept;
  ```

  返回指向第一个元素前的元素的迭代器。这个元素是作为一个占位符使用的，尝试解引用它是未定义行为。唯一的用途是用在函数`insert_after()`,`emplace_after()`, `erase_after()`, `splice_after()`，以及递增操作中，递增这个迭代器就会指向`begin()`.

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/list/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  检查容器是否为空。

### 修改元素

* [clear](https://en.cppreference.com/w/cpp/container/forward_list/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除所有的元素。

  失效所有的迭代器，但是尾后迭代器不失效。

* [insert_after](https://en.cppreference.com/w/cpp/container/forward_list/insert_after)

  ```CPP
  iterator insert_after( const_iterator pos, const T& value );
  iterator insert_after( const_iterator pos, T&& value );
  iterator insert_after( const_iterator pos, size_type count, const T& value );
  template< class InputIt >
  iterator insert_after( const_iterator pos, InputIt first, InputIt last );
  iterator insert_after( const_iterator pos, std::initializer_list<T> ilist );
  ```

  在`pos`后面插入元素。

  由于`forward_list`不能反向迭代，所以只能插入到`pos`后面。

  不会失效所有的迭代器。

* [emplace_after](https://en.cppreference.com/w/cpp/container/forward_list/emplace_after)

  ```CPP
  template< class... Args >
  iterator emplace_after( const_iterator pos, Args&&... args );
  ```

  在`pos`后插入元素，直接调用元素的构造函数进行构造，`args`就是传递给元素构造函数的参数。

  不会失效所有的迭代器。

* [erase_after](https://en.cppreference.com/w/cpp/container/forward_list/erase_after)

  ```CPP
  iterator erase_after( const_iterator pos );
  iterator erase_after( const_iterator first, const_iterator last );
  ```

  （1）移去`pos`后的元素。
  
  （2）移去`(first,list]`范围的元素，左开右闭。

* [push_front](https://en.cppreference.com/w/cpp/container/forward_list/push_front)

  ```CPP
  void push_front( const T& value );
  void push_front( T&& value );
  ```

  把`value`添加到容器的前面。

  不会失效任何迭代器。

* [emplace_front](https://en.cppreference.com/w/cpp/container/forward_list/emplace_front)

  ```CPP
  template< class... Args >
  void emplace_front( Args&&... args );
  template< class... Args >
  reference emplace_front( Args&&... args );
  ```

  在容器前面就地构建元素，使用构造函数并传递参数`args`.使用`std::forward<Args>(args)`实现完美转发。

* [pop_front](https://en.cppreference.com/w/cpp/container/forward_list/pop_front)

  ```CPP
  void pop_front();
  ```

  移除第一个元素。

  移除一个空容器的元素是未定义行为。

  只会失效指向被删除元素的迭代器。

* [resize](https://en.cppreference.com/w/cpp/container/forward_list/resize)

  ```CPP
  void resize( size_type count );
  void resize( size_type count, const value_type& value );
  ```

  修改`list`内的元素数目为`count`.

  如果大于则删除，少于则添加，等于则什么都不做。

### 其他操作

* [merge](https://en.cppreference.com/w/cpp/container/list/merge)

  ```CPP
  void merge( forward_list& other );
  void merge( forward_list&& other );
  template< class Compare >
  void merge( forward_list& other, Compare comp );
  template< class Compare >
  void merge( forward_list&& other, Compare comp )
  ```

  将两个已经排序的容器合并在一起。

  如果`other`和`*this`相等，函数不会做任何事。

  两个容器必须已排序，排序方法是`operator<`或者是`comp`.不会复制任何元素，只是修改容器中元素的指向。函数返回后，`other`变为空。

  这个操作是稳定的，不会改变相等元素的顺序，`*this`与`other`相等的元素，总是`*this`的元素在前。

  不会失效任何迭代器，但是操作完成后，所有的迭代器指向的元素已经转移到了`*this`内。

* [splice_after](https://en.cppreference.com/w/cpp/container/forward_list/splice_after)

  ```CPP
  void splice_after( const_iterator pos, forward_list& other );
  void splice_after( const_iterator pos, forward_list&& other );
  void splice_after( const_iterator pos, forward_list& other,
                    const_iterator it );
  void splice_after( const_iterator pos, forward_list&& other,
                    const_iterator it );
  void splice_after( const_iterator pos, forward_list& other,
                    const_iterator first, const_iterator last );
  void splice_after( const_iterator pos, forward_list&& other,
                    const_iterator first, const_iterator last );
  ```

  将`other`里的元素转移到`*this`中，元素会被插入到`pos`后面。

  不会实际复制和移动元素，而是修改指针的指向，不会失效迭代器，但是操作完成后，迭代器指向的被转移的函数已经转移到了`*this`内。

  `1`和`2`会把`other`里的所有元素转移到`*this`中，插入的位置是`pos`后。

  `3`和`4`会把`other`里`it`后的元素转移到`*this`中，插入的位置是`pos`后。

  `5`和`6`会把`other`里`(first,last)`指向的元素转移到`*this`中，插入的位置是`pos`后。

* [remove, remove_if](https://en.cppreference.com/w/cpp/container/forward_list/remove)

  ```CPP
  void remove( const T& value );
  template< class UnaryPredicate >
  void remove_if( UnaryPredicate p );
  ```

  移去满足条件的元素。

  `1`会把等于`value`的元素移除，使用`operator==`.

  `2`会移去所有一元谓词`p`返回为`true`的元素。

  只会失效指向被移去的元素的迭代器。

* [reverse](https://en.cppreference.com/w/cpp/container/forward_list/reverse)

  ```CPP
  void reverse() noexcept;
  ```

  反转列表元素，不会失效任何迭代器。

* [unique](https://en.cppreference.com/w/cpp/container/forward_list/unique)

  ```CPP
  void unique();
  template< class BinaryPredicate >
  void unique( BinaryPredicate p );
  ```

  从容器中删除所有**连续**的重复元素，只保留一个元素。

  `1`使用`operator==`比较元素。

  `2`使用二元谓词`p`比较元素，返回为`true`时，两元素相等。

  只会失效指向被移去的元素的迭代器。

* [sort](https://en.cppreference.com/w/cpp/container/forward_list/sort)

  ```CPP
  void sort();
  template< class Compare >
  void sort( Compare comp );
  ```

  排序元素，并保持相等元素的顺序。

  `1`使用`operator<`比较两个元素。

  `2`使用比较器`comp`比较两个元素。

  不会失效任何迭代器。
