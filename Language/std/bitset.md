# bitset

定义在`bitset`头文件中，代表了一个`N`位的比特位。

## 类原型

```CPP
template< std::size_t N >
class bitset;
```

## 描述

`std::bitset`表示`N`位的比特位，可以使用标准的数组方式来操作，也可以在字符串或整数之间转换。也支持移位操作。

## 成员类

### reference

```CPP
class reference;
```

这个成员类是保存指向单个比特位的引用，因为标准的C++程序无法指明单独一个比特。

最常用于保存`[]`操作符返回的对象。

通过`reference`对`bitset`进行读取或者写入都可能读取所有的`bitset`.

## 成员函数

### 构造函数

* [bitset](https://en.cppreference.com/w/cpp/utility/bitset/bitset)

  ```CPP
  constexpr bitset() noexcept;
  constexpr bitset( unsigned long long val ) noexcept;
  template< class CharT >
  explicit bitset( const CharT* str,
                 std::size_t n = std::size_t(-1),
                 CharT zero = CharT('0'),
                 CharT one = CharT('1') );
  ```

  默认构造函数会把所有的`bit`设置为零.`false`.

### 访问元素

* [operator[]](https://en.cppreference.com/w/cpp/utility/bitset/operator_at)

  ```CPP
  bool operator[]( std::size_t pos ) const;
  reference operator[]( std::size_t pos );
  ```

  访问特定位置的比特位。不同于`test()`,它不会检测`pos`是否超限。

* [test](https://en.cppreference.com/w/cpp/utility/bitset/test)

  ```CPP
  bool test( std::size_t pos ) const;
  ```

  获取特定位置的比特值，带有边界检查.

* [all,any,none](https://en.cppreference.com/w/cpp/utility/bitset/all_any_none)

  ```CPP
  bool all() const;
  bool any() const;
  bool none() const;
  ```

  `all`检查是否所有比特位均为`true`.

  `any`检查是否有比特位为`true`.

  `none`检查是否所有比特位均为`false`.

* [count](https://en.cppreference.com/w/cpp/utility/bitset/count)

  ```CPP
  std::size_t count() const;
  ```

  检查`bitset`中为`true`的比特位的个数。

### 容量

* [size](https://en.cppreference.com/w/cpp/utility/bitset/size)

  ```CPP
  std::size_t size() const;
  ```

  返回模板实参`N`.

### 修改元素

* [operator&=,|=,^=,~](https://en.cppreference.com/w/cpp/utility/bitset/operator_logic)

  ```CPP
  bitset& operator&=( const bitset& other );
  bitset& operator|=( const bitset& other );
  bitset& operator^=( const bitset& other );
  bitset operator~() const;
  ```

  进行按位与，或，异或.`other`的`N`必须与`this`的`N`一致.

* [operator<<,<<=,>>,>>=](https://en.cppreference.com/w/cpp/utility/bitset/operator_ltltgtgt)

  ```CPP
  bitset operator<<( std::size_t pos ) const;
  bitset& operator<<=( std::size_t pos );
  bitset operator>>( std::size_t pos ) const;
  bitset& operator>>=( std::size_t pos );
  ```

  进行移位操作，超出的位被丢弃，新加的位为零.

* [set](https://en.cppreference.com/w/cpp/utility/bitset/set)

  ```CPP
  bitset& set();
  bitset& set( std::size_t pos, bool value = true );
  ```

  `1`设置所有位为真

  `2`设置`pos`处的比特为`value`.

* [reset](https://en.cppreference.com/w/cpp/utility/bitset/reset)

  ```CPP
  bitset& reset();
  bitset& reset( std::size_t pos );
  ```

  `1`设置所有位为`false`.

  `2`设置`pos`处的比特位为`false`.

* [flip](https://en.cppreference.com/w/cpp/utility/bitset/flip)

  ```CPP
  bitset& flip();
  bitset& flip( std::size_t pos );
  ```

  翻转所有位或者特定`pos`.

### 转换

* [to_string](https://en.cppreference.com/w/cpp/utility/bitset/to_string)

  ```CPP
  template<
      class CharT = char,
      class Traits = std::char_traits<CharT>,
      class Allocator = std::allocator<CharT>
  >
  std::basic_string<CharT, Traits, Allocator>
      to_string( CharT zero = CharT('0'),
                CharT one = CharT('1') ) const;
  ```

  转换为字符串,同时使用`zero`表示`0`,`one`表示`1`.

* [to_ulong,to_ullong](https://en.cppreference.com/w/cpp/utility/bitset/to_ulong)

  ```CPP
  unsigned long to_ulong() const;
  unsigned long long to_ullong() const;
  ```

  转换为`unsigned long`或者是`unsigned long long`.如果无法转换，则抛出异常。
