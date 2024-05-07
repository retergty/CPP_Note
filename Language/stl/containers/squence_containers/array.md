# array

参考文档

* CPP reference [std::array](https://en.cppreference.com/w/cpp/container/array)

定义在头文件`<array>`.

## 类原型

```CPP
template<
    class T,
    std::size_t N
> struct array;
```

## 描述

`std::array`是一个包含固定元素的数组。底层实现通常为。

```CPP
template<typename T, std::size_t N>
struct array
{
    T data[N];
};
```

这个容器是一个聚合类，和一个包含`C`风格的数组`T[]`作为唯一的非静态成员的结构体一样。但是不同于`C`风格的数组，它不会自动转换为指向数组头元素的指针`T*`.

这个类型保持了`C`风格的数组的性能与易用性，同时拥有标准库的优势，比如知道自己的大小，支持赋值运算以及随机访问迭代器等。

## 迭代器类型

迭代器类型为随机访问迭代器`Random Access Iterator`.

### 会使得迭代器失效的操作

任何操作都不会使得迭代器失效。

## 常用成员函数

### 构建容器

支持聚合类的初始化方法。

```CPP
std::array<int, 3> a = {1, 2, 3};
```

### 访问数据

* [at](https://en.cppreference.com/w/cpp/container/array/at)

  通过边界检查访问指定元素。

  ```CPP
  reference at( size_type pos );
  const_reference at( size_type pos ) const;
  ```

  如果`pos`不在容器的范围内，抛出异常。

* [data](https://en.cppreference.com/w/cpp/container/array/data)

  直接访问底层的连续存储空间。

  ```CPP
  T* data() noexcept;
  const T* data() const;
  ```

  返回指向底层存储空间首元素的指针，指针合法范围为`[data(), data() + size())`

### 访问容量

* [empty](https://en.cppreference.com/w/cpp/container/array/empty)

  ```CPP
  bool empty() const noexcept;
  ```

  返回容器是否为空。

* [size](https://en.cppreference.com/w/cpp/container/array/size)

  ```CPP
  size_type size() const noexcept;
  ```

  返回容器当前存储的元素个数。也就是模板参数`std::size_t N`.

* [max_size](https://en.cppreference.com/w/cpp/container/array/max_size)

  ```CPP
  constexpr size_type max_size() const noexcept;
  ```

  返回容器最大存储个数，也就是模板参数`std::size_t N`.

### 修改容器

* [fill](https://en.cppreference.com/w/cpp/container/array/fill)

  给所有的元素赋值为`value`.

  ```CPP
  void fill( const T& value );
  ```

* [swap](https://en.cppreference.com/w/cpp/container/array/swap)

  ```CPP
  void swap( array& other ) noexcept(/* see below */);
  ```

  交换两个容器的内容，不会失效任何迭代器。
