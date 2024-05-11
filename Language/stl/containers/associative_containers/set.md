# set

参考文档

* CPP REFERENCE[std::set](https://en.cppreference.com/w/cpp/container/set)

定义在头文件`<set>`.

## 类原型

```CPP
template<
    class Key,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<Key>
> class set;
namespace pmr {
    template<
        class Key,
        class Compare = std::less<Key>
    > using set = std::set<Key, Compare, std::pmr::polymorphic_allocator<Key>>;
}
```

## 描述

`set`是一个关联容器，存储了指定数据类型的有序唯一元素，排序是使用可调用对象`Compare`实现的，默认是对元素升序排列。

`set`通常实现为红黑树，在`set`中查找，删除，插入操作复杂度都是`O(logN)`。

两个元素被认为是相等的，当且仅当表达式`!comp(a, b) && !comp(b, a)`为真。

## set的迭代器

`set`的迭代器类型是`LegacyBidirectionalIterator`.

### 会使得迭代器失效的操作

* 插入元素不会失效任何迭代器
* 删除元素只会失效指向删除元素的迭代器

## 常用成员函数

有时成员函数会接受指向插入位置的迭代器，注意，`set`总是会维护元素的有序性，这意味着`set`会尽可能近地在指定位置插入元素，换句话说，就是从这里开始比较元素，如果大致知道了元素的插入位置，可以加快插入的速度。

### 构建容器

* [set](https://en.cppreference.com/w/cpp/container/set/set)

* [operator=](https://en.cppreference.com/w/cpp/container/set/operator%3D)

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/set/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/set/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。

### 修改容器

* [clear](https://en.cppreference.com/w/cpp/container/set/clear)

  ```CPP
  void clear() noexcept;
  ```

  清除`set`中所有的元素，调用完毕后，`size()`为`0`.

  失效所有的迭代器。除了尾后迭代器。

* [insert](https://en.cppreference.com/w/cpp/container/set/insert)

  ```CPP
  std::pair<iterator, bool> insert( const value_type& value );
  std::pair<iterator, bool> insert( value_type&& value );
  iterator insert( const_iterator pos, const value_type& value );
  iterator insert( const_iterator pos, value_type&& value );
  template< class InputIt >
  void insert( InputIt first, InputIt last );
  void insert( std::initializer_list<value_type> ilist );
  insert_return_type insert( node_type&& nh );
  iterator insert( const_iterator pos, node_type&& nh );
  ```

  如果容器没有包含相同的`value`,则把`value`插入到容器中

  `1`,`2`插入`value`,返回一个`std::pair`包含指向插入元素的迭代器以及是否插入。

  `3`,`4`尽可能在`pos`前的最近距离插入`value`.

  `5`利用输入迭代器，把`[first,last)`范围的元素插入到容器，如果范围内有两个相等的元素，具体插入的是哪个元素是未指定的，

  `6`将初始化列表`ilist`的元素插入到容器。

  `7`将节点`nh`中的元素插入到容器中。

  不会失效任何迭代器。

* [emplace](https://en.cppreference.com/w/cpp/container/set/emplace)

  ```CPP
  template< class... Args >
  std::pair<iterator, bool> emplace( Args&&... args );
  ```

  使用参数`args`在容器内原地构建元素，如果容器已经有相等元素了，那么构建的元素会立即析构。

  不会失效任何迭代器。

* [emplace_hint](https://en.cppreference.com/w/cpp/container/set/emplace_hint)

  ```CPP
  template< class... Args >
  iterator emplace_hint( const_iterator hint, Args&&... args );
  ```

  将一个元素插入到容器中，使其尽可能与`hint`指向元素的前面最近。在知道了大致的插入位置时，可以加快插入的速度（复杂度变为常数）。

  不会失效任何迭代器。

  返回值指向插入元素的迭代器，或者是已经存在的相等的元素的迭代器。

  参考文档中讲解了`emplace_hint`插入的速度比较，可见，往正确的位置插入时，时间大幅缩短。注意，如果插入的元素刚好在`hint`右边，也是只需要一次比较即可插入。

* [erase](https://en.cppreference.com/w/cpp/container/set/erase)

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

* [swap](https://en.cppreference.com/w/cpp/container/set/swap)

  函数原型为

  ```CPP
  void swap( set& other ) noexcept;
  ```

  交换两个`set`的内容与容量，**不会调用**任何容器内元素`move`,`copy`,`swap`操作。

  除了`end()`迭代器失效，任何其他的迭代器与引用均不失效，指向原来的位置。（但是所属的`set`不同了）。

* [extract](https://en.cppreference.com/w/cpp/container/set/extract)

  ```CPP
  node_type extract( const_iterator position );
  node_type extract( const Key& k );
  ```

  提取出指定迭代器或者是指定关键字的元素所在的节点。

  不会调用元素的移动或者是复制函数，而是改变容器内部指针的指向(可能会发生重平衡)。

  提取出一个节点只会失效指向这个元素的迭代器，但是指向这个元素的引用和指针**不会失效**。