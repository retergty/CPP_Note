# map

参考文档

* CPP REFERENCE[std::map](https://en.cppreference.com/w/cpp/container/map)

定义在头文件`<map>`.

## 类原型

```CPP
template<
    class Key,
    class T,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>
> class map;
namespace pmr {
    template<
        class Key,
        class T,
        class Compare = std::less<Key>
    > using map = std::map<Key, T, Compare,
                           std::pmr::polymorphic_allocator<std::pair<const Key, T>>>;
}
```

## 描述

`map`是一个有序关联容器，其中包含具有唯一关键字的对`pair`。排序是使用可调用对象`Compare`实现的，对元素升序排列。

`map`通常实现为红黑树，在`map`中查找，删除，插入操作复杂度都是`O(logN)`。

两个元素被认为是相等的，当且仅当表达式`!comp(a, b) && !comp(b, a)`为真。

`map`的元素类型是`std::pair<const Key, T>`.元素的相等比较是通过关键字进行的。

## map的迭代器

`map`的迭代器类型是`LegacyBidirectionalIterator`.

### 会使得迭代器失效的操作

* 插入元素不会失效任何迭代器
* 删除元素只会失效指向删除元素的迭代器

## 常用成员函数

有时成员函数会接受指向插入位置的迭代器，注意，`map`总是会维护元素的有序性，这意味着`map`会尽可能近地在指定位置插入元素，换句话说，就是从这里开始比较元素，如果大致知道了元素的插入位置，可以加快插入的速度。

### 构建容器

* [map](https://en.cppreference.com/w/cpp/container/map/map)

* [operator=](https://en.cppreference.com/w/cpp/container/map/operator%3D)

### 访问元素

* [at](https://en.cppreference.com/w/cpp/container/map/at)

  ```CPP
  T& at( const Key& key );
  const T& at( const Key& key ) const;
  ```

  返回关键字为`key`所对应的值的引用，如果容器中没有`key`关键字的元素，抛出异常。

* [operator[]](https://en.cppreference.com/w/cpp/container/map/operator_at)

  ```CPP
  T& operator[]( const Key& key );
  T& operator[]( Key&& key );
  ```

  返回关键字为`key`所对应的值的引用，如果没有，则插入。使用`T`的默认构造函数。

  `operator[]`会在不存在这个`key`时插入，如果不希望插入，可以使用`at`.

  ```CPP
  letter_counts['b'] = 42; // updates an existing value
  letter_counts['x'] = 9;  // inserts a new value
  word_map["that"]; // just inserts the pair {"that", 0}
  ```

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/map/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/map/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。

### 修改容器

* [clear](https://en.cppreference.com/w/cpp/container/map/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除`map`中所有的元素，调用完毕后，`size()`为`0`.

  失效所有的迭代器。除了尾后迭代器。

* [insert](https://en.cppreference.com/w/cpp/container/set/insert)

  ```CPP
  std::pair<iterator, bool> insert( const value_type& value );
  template< class P >
  std::pair<iterator, bool> insert( P&& value );
  std::pair<iterator, bool> insert( value_type&& value );
  iterator insert( const_iterator pos, const value_type& value );
  template< class P >
  iterator insert( const_iterator pos, P&& value );
  iterator insert( const_iterator pos, value_type&& value );
  template< class InputIt >
  void insert( InputIt first, InputIt last );
  void insert( std::initializer_list<value_type> ilist );
  insert_return_type insert( node_type&& nh );
  iterator insert( const_iterator pos, node_type&& nh );
  ```

  如果容器没有包含相同的`key`,则把`value`插入到容器中。使用返回值检验是否成功插入。

  只有当模板实参`P`可以构建`value_type`时，相应的`insert`才会加入到重载决议中。

  不会失效任何迭代器。

* [emplace](https://en.cppreference.com/w/cpp/container/map/emplace)

  ```CPP
  template< class... Args >
  std::pair<iterator, bool> emplace( Args&&... args );
  ```

  使用参数`args`在容器内原地构建元素，如果容器已经有相等元素了，那么构建的元素会立即析构。

  不会失效任何迭代器。

* [emplace_hint](https://en.cppreference.com/w/cpp/container/map/emplace_hint)

  ```CPP
  template< class... Args >
  iterator emplace_hint( const_iterator hint, Args&&... args );
  ```

  将一个元素插入到容器中，使用参数就地构建`std::pair<const Key, T>`,使其尽可能与`hint`指向元素的前面最近。在知道了大致的插入位置时，可以加快插入的速度（复杂度变为常数）。

  不会失效任何迭代器。

  返回值指向插入元素的迭代器，或者是已经存在的相等的元素的迭代器。

  参考文档中讲解了`emplace_hint`插入的速度比较，可见，往正确的位置插入时，时间大幅缩短。注意，如果插入的元素刚好在`hint`右边，也是只需要一次比较即可插入。

* [try_emplace](https://en.cppreference.com/w/cpp/container/map/try_emplace)

  ```CPP
  template< class... Args >
  std::pair<iterator, bool> try_emplace( const Key& k, Args&&... args );
  template< class... Args >
  std::pair<iterator, bool> try_emplace( Key&& k, Args&&... args );
  template< class K, class... Args >
  std::pair<iterator, bool> try_emplace( K&& k, Args&&... args );
  template< class... Args >
  iterator try_emplace( const_iterator hint, const Key& k, Args&&... args );
  template< class... Args >
  iterator try_emplace( const_iterator hint, Key&& k, Args&&... args );
  ```

  如果关键字`key`已经存在于容器中，什么也不做。否则插入一个新的元素，就地构建`T`。

* [erase](https://en.cppreference.com/w/cpp/container/map/erase)

  ```CPP
  iterator erase( iterator pos );
  iterator erase( const_iterator pos );
  iterator erase( iterator first, iterator last );
  iterator erase( const_iterator first, const_iterator last );
  size_type erase( const Key& key );
  ```

  `1`,`2`删除`pos`指向的元素。

  `3`，`4`擦除`[first,last)`范围指向的元素。

  `5`擦除关键字为`key`的元素，没有则不做，返回值为擦除元素的个数（1或0）。

  指向被擦除元素的迭代器会失效，其它位置的迭代器不会失效。

* [swap](https://en.cppreference.com/w/cpp/container/map/swap)

  函数原型为

  ```CPP
  void swap( map& other ) noexcept;
  ```

  交换两个`map`的内容与容量，**不会调用**任何容器内元素`move`,`copy`,`swap`操作。

  除了`end()`迭代器失效，任何其他的迭代器与引用均不失效，指向原来的位置。（但是所属的`map`不同了）。

* [extract](https://en.cppreference.com/w/cpp/container/map/extract)

  ```CPP
  node_type extract( const_iterator position );
  node_type extract( const Key& k );
  ```

  提取出指定迭代器或者是指定关键字的元素所在的节点。

  不会调用元素的移动或者是复制函数，而是改变容器内部指针的指向(可能会发生重平衡)。

  提取出一个节点只会失效指向这个元素的迭代器，但是指向这个元素的引用和指针**不会失效**,但是当这个元素被节点句柄拥有时**不能使用**，只有当其插入到容器后才能使用。否则会违反严格别名规则，带来未定义行为。

* [merge](https://en.cppreference.com/w/cpp/container/map/merge)

  ```CPP
  template< class C2 >
  void merge( std::map<Key, T, C2, Allocator>& source );
  template< class C2 >
  void merge( std::map<Key, T, C2, Allocator>&& source );
  template< class C2 >
  void merge( std::multimap<Key, T, C2, Allocator>& source );
  template< class C2 >
  void merge( std::multimap<Key, T, C2, Allocator>&& source );
  ```

  将`source`中的每个元素提取出来，并插入到`*this`中，并使用`*this`的比较器，如果`source`中有和`*this`相等的元素，这个元素不会被提取出来。不会调用元素的移动或者是复制函数，而是改变容器内部指针的指向(可能会发生重平衡)。不会失效任何迭代器，但是现在指向了`*this`里的元素了。

### 查找

* [count](https://en.cppreference.com/w/cpp/container/map/count)

  ```CPP
  size_type count( const Key& key ) const;
  template< class K >
  size_type count( const K& x ) const;
  ```

  返回满足关键字`key`的元素的数目。

  `1`只会返回0或1，因为`map`不允许相同关键字的元素重复出现。

  `2`是使用一个比较器`x`.

* [find](https://en.cppreference.com/w/cpp/container/map/find)

  ```CPP
  iterator find( const Key& key );
  const_iterator find( const Key& key ) const;
  template< class K >
  iterator find( const K& x );
  template< class K >
  const_iterator find( const K& x ) const;
  ```

  查找满足关键字的元素，返回指向这个元素的迭代器，若没有，则返回尾后迭代器。

* [contains](https://en.cppreference.com/w/cpp/container/map/contains)

  ```CPP
  bool contains( const Key& key ) const;
  template< class K >
  bool contains( const K& x ) const;
  ```

  检查是否有满足关键字的元素，若有，则返回`true`，若无，则返回`false`.

* [equal_range](https://en.cppreference.com/w/cpp/container/map/equal_range)

  ```CPP
  std::pair<iterator, iterator> equal_range( const Key& key );
  std::pair<const_iterator, const_iterator> equal_range( const Key& key ) const;
  template< class K >
  std::pair<iterator, iterator> equal_range( const K& x );
  template< class K >
  std::pair<const_iterator, const_iterator> equal_range( const K& x ) const;
  ```

  返回一个范围，这个范围包含所有与关键字相等的元素，这个范围由两个迭代器组成，第一个迭代器指向第一个不小于`key`的元素，第二个迭代器指向第一个大于`key`的元素。如果没有则返回尾后迭代器。

* [lower_bound](https://en.cppreference.com/w/cpp/container/map/lower_bound)

  ```CPP
  iterator lower_bound( const Key& key );
  const_iterator lower_bound( const Key& key ) const;
  template< class K >
  iterator lower_bound( const K& x );
  template< class K >
  const_iterator lower_bound( const K& x ) const;
  ```

  返回第一个不小于指定关键字的元素的迭代器。对于`set`来说就是对应的元素的迭代器，若没有，则返回尾后迭代器。

* [upper_bound](https://en.cppreference.com/w/cpp/container/map/upper_bound)

  ```CPP
  iterator upper_bound( const Key& key );
  const_iterator upper_bound( const Key& key ) const;
  template< class K >
  iterator upper_bound( const K& x );
  template< class K >
  const_iterator upper_bound( const K& x ) const;
  ```

  返回第一个大于指定关键字的元素的迭代器。若没有，则返回尾后迭代器。
  