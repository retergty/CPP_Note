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

`set`是一个关联容器，存储了指定数据类型的有序唯一元素，排序是使用可调用对象`Compare`实现的。

`set`通常实现为红黑树，在`set`中查找，删除，插入操作复杂度都是`O(logN)`。

两个元素被认为是相等的，当且仅当表达式`!comp(a, b) && !comp(b, a)`为真。

## set的迭代器

`set`的迭代器类型是`LegacyBidirectionalIterator`.

### 会使得迭代器失效的操作

* 插入元素不会失效任何迭代器
* 删除元素只会失效指向删除元素的迭代器

## 常用成员函数

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

