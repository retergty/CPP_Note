# multiset

参考文档

* CPP REFERENCE[std::multiset](https://en.cppreference.com/w/cpp/container/multiset)

定义在头文件`<set>`.

## 类原型

```CPP
template<
    class Key,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<Key>
> class multiset;
namespace pmr {
    template<
        class Key,
        class Compare = std::less<Key>
    > using multiset = std::multiset<Key, Compare, std::pmr::polymorphic_allocator<Key>>;
}
```

## 描述

`multiset`是一个有序关联容器，存储了指定数据类型的有序唯一元素，排序是使用可调用对象`Compare`实现的，对元素升序排列。

不同于`set`，`multiset`允许相同关键字的元素重复出现。

`multiset`通常实现为红黑树，在`multiset`中查找，删除，插入操作复杂度都是`O(logN)`。

两个元素被认为是相等的，当且仅当表达式`!comp(a, b) && !comp(b, a)`为真。

## multiset的迭代器

`multiset`的迭代器类型是`LegacyBidirectionalIterator`.

### 会使得迭代器失效的操作

* 插入元素不会失效任何迭代器
* 删除元素只会失效指向删除元素的迭代器

## 常用成员函数

有时成员函数会接受指向插入位置的迭代器，注意，`multiset`总是会维护元素的有序性，这意味着`multiset`会尽可能近地在指定位置插入元素，换句话说，就是从这里开始比较元素，如果大致知道了元素的插入位置，可以加快插入的速度。

### 构建容器

* [multiset](https://en.cppreference.com/w/cpp/container/multiset/multiset)

* [operator=](https://en.cppreference.com/w/cpp/container/multiset/operator%3D)

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/multiset/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/multiset/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。

### 修改容器

* [clear](https://en.cppreference.com/w/cpp/container/multiset/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除`set`中所有的元素，调用完毕后，`size()`为`0`.

  失效所有的迭代器。除了尾后迭代器。

* [insert](https://en.cppreference.com/w/cpp/container/multiset/insert)

  ```CPP
  iterator insert( const value_type& value ); 

  iterator insert( value_type&& value );

  iterator insert( iterator pos, const value_type& value ); 
  iterator insert( const_iterator pos, const value_type& value );

  iterator insert( const_iterator pos, value_type&& value );

  template< class InputIt >
  void insert( InputIt first, InputIt last );

  void insert( std::initializer_list<value_type> ilist );

  iterator insert( node_type&& nh );

  iterator insert( const_iterator pos, node_type&& nh );
  ```

  则把`value`插入到容器中，同时保持相等元素的顺序

  `1`,`2`插入`value`,返回指向插入元素的迭代器，如果有相等的元素，插入在相等元素的末尾。

  `3`,`4`尽可能在`pos`前的最近距离插入`value`.

  `5`利用输入迭代器，把`[first,last)`范围的元素插入到容器。

  `6`将初始化列表`ilist`的元素插入到容器。

  `7`将节点`nh`中的元素插入到容器中。如果节点是空节点，不会进行任何操作。

  不会失效任何迭代器。

* [emplace](https://en.cppreference.com/w/cpp/container/multiset/emplace)

  ```CPP
  template< class... Args >
  iterator emplace( Args&&... args );
  ```

  使用参数`args`在容器内原地构建元素.

  不会失效任何迭代器。

* [emplace_hint](https://en.cppreference.com/w/cpp/container/multiset/emplace_hint)

  ```CPP
  template< class... Args >
  iterator emplace_hint( const_iterator hint, Args&&... args );
  ```

  将一个元素插入到容器中，使其尽可能与`hint`指向元素的前面最近。在知道了大致的插入位置时，可以加快插入的速度（复杂度变为常数）。

  不会失效任何迭代器。

  返回值指向插入元素的迭代器.

  参考文档中讲解了`emplace_hint`插入的速度比较，可见，往正确的位置插入时，时间大幅缩短。注意，如果插入的元素刚好在`hint`右边，也是只需要一次比较即可插入。

* [erase](https://en.cppreference.com/w/cpp/container/multiset/erase)

  ```CPP
  iterator erase( iterator pos );
  iterator erase( const_iterator pos );
  iterator erase( iterator first, iterator last );
  iterator erase( const_iterator first, const_iterator last );
  size_type erase( const Key& key );
  ```

  `1`,`2`删除`pos`指向的元素。

  `3`，`4`擦除`[first,last)`范围指向的元素。

  返回指向被擦除元素后边的一个元素的迭代器。

  `5`擦除关键字为`key`的元素，没有则不做，返回值为擦除元素的个数。

  指向被擦除元素的迭代器会失效，其它位置的迭代器不会失效。

* [swap](https://en.cppreference.com/w/cpp/container/multiset/swap)

  函数原型为

  ```CPP
  void swap( set& other ) noexcept;
  ```

  交换两个`multiset`的内容与容量，**不会调用**任何容器内元素`move`,`copy`,`swap`操作。

  除了`end()`迭代器失效，任何其他的迭代器与引用均不失效，指向原来的位置。（但是所属的`multiset`不同了）。

* [extract](https://en.cppreference.com/w/cpp/container/multiset/extract)

  ```CPP
  node_type extract( const_iterator position );
  node_type extract( const Key& k );
  ```

  提取出指定迭代器或者是指定关键字的元素所在的节点。

  不会调用元素的移动或者是复制函数，而是改变容器内部指针的指向(可能会发生重平衡)。

  提取出一个节点只会失效指向这个元素的迭代器，但是指向这个元素的引用和指针**不会失效**,但是当这个元素被节点句柄拥有时**不能使用**，只有当其插入到容器后才能使用。否则会违反严格别名规则，带来未定义行为。

* [merge](https://en.cppreference.com/w/cpp/container/multiset/merge)

  ```CPP
  template< class C2 >
  void merge( std::set<Key, C2, Allocator>& source );
  template< class C2 >
  void merge( std::set<Key, C2, Allocator>&& source );
  template< class C2 >
  void merge( std::multiset<Key, C2, Allocator>& source );
  template< class C2 >
  void merge( std::multiset<Key, C2, Allocator>&& source );
  ```

  将`source`中的每个元素提取出来，并插入到`*this`中，并使用`*this`的比较器，不会调用元素的移动或者是复制函数，而是改变容器内部指针的指向(可能会发生重平衡)。不会失效任何迭代器，但是现在指向了`*this`里的元素了。

### 查找

* [count](https://en.cppreference.com/w/cpp/container/multiset/count)

  ```CPP
  size_type count( const Key& key ) const;
  template< class K >
  size_type count( const K& x ) const;
  ```

  返回满足关键字`key`的元素的数目。

* [find](https://en.cppreference.com/w/cpp/container/multiset/find)

  ```CPP
  iterator find( const Key& key );
  const_iterator find( const Key& key ) const;
  template< class K >
  iterator find( const K& x );
  template< class K >
  const_iterator find( const K& x ) const;
  ```

  查找满足关键字的元素，返回指向这个元素的迭代器，如果有多个满足这个关键字的元素，它们中的任何一个都有可能被返回,若没有，则返回尾后迭代器。

* [contains](https://en.cppreference.com/w/cpp/container/multiset/contains)

  ```CPP
  bool contains( const Key& key ) const;
  template< class K >
  bool contains( const K& x ) const;
  ```

  检查是否有满足关键字的元素，若有，则返回`true`，若无，则返回`false`.

* [equal_range](https://en.cppreference.com/w/cpp/container/multiset/equal_range)

  ```CPP
  std::pair<iterator, iterator> equal_range( const Key& key );
  std::pair<const_iterator, const_iterator> equal_range( const Key& key ) const;
  template< class K >
  std::pair<iterator, iterator> equal_range( const K& x );
  template< class K >
  std::pair<const_iterator, const_iterator> equal_range( const K& x ) const;
  ```

  返回一个范围，这个范围包含所有与关键字相等的元素，这个范围由两个迭代器组成，第一个迭代器指向第一个不小于`key`的元素，第二个迭代器指向第一个大于`key`的元素。如果没有则返回尾后迭代器。

* [lower_bound](https://en.cppreference.com/w/cpp/container/multiset/lower_bound)

  ```CPP
  iterator lower_bound( const Key& key );
  const_iterator lower_bound( const Key& key ) const;
  template< class K >
  iterator lower_bound( const K& x );
  template< class K >
  const_iterator lower_bound( const K& x ) const;
  ```

  返回第一个不小于指定关键字的元素的迭代器。

* [upper_bound](https://en.cppreference.com/w/cpp/container/multiset/upper_bound)

  ```CPP
  iterator upper_bound( const Key& key );
  const_iterator upper_bound( const Key& key ) const;
  template< class K >
  iterator upper_bound( const K& x );
  template< class K >
  const_iterator upper_bound( const K& x ) const;
  ```

  返回第一个大于指定关键字的元素的迭代器。若没有，则返回尾后迭代器。
