# span

定义在头文件 `<span>` 中的 `std::span` 是一个轻量级的视图类型，用于表示连续内存中的一段数据。它提供了一种安全且高效的方式来访问数组、`std::vector`、`std::array` 等容器中的元素，而无需复制数据。

参考文档：

* [std::span](https://en.cppreference.com/w/cpp/container/span)

## 类原型

```CPP
template< class ElementType, std::size_t Extent = dynamic_extent >
class span;
```

## 描述

`std::span` 是一个模板类，接受两个参数：`ElementType` 表示元素类型，`Extent` 表示跨度的大小。默认情况下，`Extent` 是 `dynamic_extent`，表示跨度的大小在运行时确定。

## 成员函数

### 构造函数

* [span](https://en.cppreference.com/w/cpp/container/span/span)

  ```CPP
    constexpr span() noexcept;

    template< class It >
    explicit(extent != std::dynamic_extent)
    constexpr span( It first, size_type count );

    template< class It, class End >
    explicit(extent != std::dynamic_extent)
    constexpr span( It first, End last );

    template< std::size_t N >
    constexpr span( std::type_identity_t<element_type> (&arr)[N] ) noexcept;

    template< class U, std::size_t N >
    constexpr span( std::array<U, N>& arr ) noexcept;

    template< class U, std::size_t N >
    constexpr span( const std::array<U, N>& arr ) noexcept;

    template< class R >
    explicit(extent != std::dynamic_extent)
    constexpr span( R&& r );

    explicit(extent != std::dynamic_extent)
    constexpr span( std::initializer_list<value_type> il ) noexcept;

    template< class U, std::size_t N >
    explicit(extent != std::dynamic_extent && N == std::dynamic_extent)
    constexpr span( const std::span<U, N>& source ) noexcept;

    constexpr span( const span& other ) noexcept = default;
  ```

  构造一个 `std::span` 对象。可以从指针和大小、两个迭代器、数组、`std::array`、其他 `std::span` 等多种方式构造。其中(7)构造函数接受一个范围类型 `R`，如果 `R` 满足 `std::ranges::contiguous_range` 和 `std::ranges::viewable_range` 的要求，并且其元素类型可以转换为 `element_type`，则可以使用该构造函数。

### 访问元素

* [operator[]](https://en.cppreference.com/w/cpp/container/span/operator_at)

  ```CPP
  constexpr reference operator[]( size_type idx ) const;
  ```

  返回索引 `idx` 处的元素的引用。调用者必须保证 `idx` 小于跨度的大小。

* [front](https://en.cppreference.com/w/cpp/container/span/front)

  ```CPP
  constexpr reference front() const;
  ```

  返回跨度的第一个元素的引用。调用者必须保证跨度不为空。

* [back](https://en.cppreference.com/w/cpp/container/span/back)

  ```CPP
  constexpr reference back() const;
  ```

  返回跨度的最后一个元素的引用。调用者必须保证跨度不为空。

* [data](https://en.cppreference.com/w/cpp/container/span/data)

  ```CPP
  constexpr pointer data() const noexcept;
  ```

  返回指向跨度第一个元素的指针。如果跨度为空，返回 `nullptr`。

### 大小和容量

* [size](https://en.cppreference.com/w/cpp/container/span/size)

  ```CPP
  constexpr size_type size() const noexcept;
  ```

  返回跨度中元素的数量。

* [size_bytes](https://en.cppreference.com/w/cpp/container/span/size_bytes)

  ```CPP
  constexpr size_type size_bytes() const noexcept;
  ```

  返回跨度中元素占用的字节数。

* [empty](https://en.cppreference.com/w/cpp/container/span/empty)

  ```CPP
  constexpr bool empty() const noexcept;
  ```

  如果跨度不包含任何元素，返回 `true`；否则返回 `false`。

### 子跨度

* [first](https://en.cppreference.com/w/cpp/container/span/first)

  ```CPP
  template< std::size_t Count >
  constexpr span<element_type, Count> first() const;

  constexpr span<element_type, dynamic_extent> first( size_type count ) const;
  ```

  返回一个新的 `std::span`，包含当前跨度的前`Count`个元素。调用者必须保证 `Count` 或 `count` 小于或等于当前跨度的大小。

* [last](https://en.cppreference.com/w/cpp/container/span/last)

  ```CPP
  template< std::size_t Count >
  constexpr std::span<element_type, Count> last() const;

  constexpr std::span<element_type, std::dynamic_extent>
      last( size_type count ) const;
  ```

  返回一个新的 `std::span`，包含当前跨度的后`Count`个元素。调用者必须保证 `Count` 或 `count` 小于或等于当前跨度的大小。

* [subspan](https://en.cppreference.com/w/cpp/container/span/subspan)

  ```CPP
  template< std::size_t Offset, std::size_t Count = dynamic_extent >
  constexpr span<element_type, Count> subspan() const;

  constexpr std::span<element_type, std::dynamic_extent>
    subspan( size_type offset,
             size_type count = std::dynamic_extent ) const;
  ```

  返回一个新的 `std::span`，包含当前跨度从 `offset` 开始的 `count` 个元素。调用者必须保证 `offset` 小于或等于当前跨度的大小，并且 `count` 小于或等于当前跨度的大小减去 `offset`。

## 例子

```CPP
#include <span>
#include <vector>
#include <iostream>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::span<int> s(vec);

    for (size_t i = 0; i < s.size(); ++i) {
        std::cout << s[i] << " ";
    }
    std::cout << std::endl;

    auto sub = s.subspan(1, 3);
    for (size_t i = 0; i < sub.size(); ++i) {
        std::cout << sub[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
```
  