# variant

在头文件`<variant>`中定义，代表一个类型安全的联合体。

参考文档

* CPP Reference[variant](https://en.cppreference.com/w/cpp/utility/variant)

## 类声明

```CPP
template< class... Types >
class variant;
```

* `Types`就是`variant`可以接受的类型。

## 描述

这个类可以用来保存许多通过模板实参指定的不同类型的值，类似于联合体。它可以保存的类型成为可选类型(`alternative types`).在任何时刻，`std::variant`要不保存着一个可选类型的值，或者是错误(这个情况极少出现)。

也就是说，哪怕是默认构造函数，`std::variant`都会保存着它**第一个**可选类型的默认构造值，如果这个类型没有默认构造函数，那么对应的`std::variant`也没有默认构造函数，可以使用`std::monostate`作为它第一个可选类型来获得默认构造的能力。

如同联合体一样，`variant`保存的对象的值的位置是其内部，不会进行动态内存分配。

`variant`不允许保存引用，数组，以及`void`类型。

`variant`允许多次保存相同类型，并保存相同类型的不同`cv`限定版本。

不允许定义`variant`的偏特化与全特化。

## 成员函数

### 构造与析构

* [variant](https://en.cppreference.com/w/cpp/utility/variant/variant)

* [~variant](https://en.cppreference.com/w/cpp/utility/variant/%7Evariant)

* [operator=](https://en.cppreference.com/w/cpp/utility/variant/operator%3D)

### 观测

* [index](https://en.cppreference.com/w/cpp/utility/variant/index)

  ```CPP
  constexpr std::size_t index() const noexcept;
  ```

  返回一个从`0`开始的索引，表示当前`variant`所保存值的类型。

  如果`variant`是`valueless_by_exception`,则返回`variant_npos`

* [valueless_by_exception](https://en.cppreference.com/w/cpp/utility/variant/valueless_by_exception)

  ```CPP
  constexpr bool valueless_by_exception() const noexcept;
  ```

  检查是否`variant`是无效的状态。

### 修改

* [emplace](https://en.cppreference.com/w/cpp/utility/variant/emplace)

  ```CPP
  template< class T, class... Args >
  T& emplace( Args&&... args );

  (constexpr since C++20)
  template< class T, class U, class... Args >
  T& emplace( std::initializer_list<U> il, Args&&... args );

  (constexpr since C++20)
  template< std::size_t I, class... Args >
  std::variant_alternative_t<I, variant>& emplace( Args&&... args );

  template< std::size_t I, class U, class... Args >
  std::variant_alternative_t<I, variant>&
      emplace( std::initializer_list<U> il, Args&&... args );
  ```

  在`variant`里按照参数就地构建值。

* [swap](https://en.cppreference.com/w/cpp/utility/variant/swap)

  ```CPP
  void swap( variant& rhs ) noexcept;
  ```

  交换两个`variant`.

## 非成员函数

* [get](https://en.cppreference.com/w/cpp/utility/variant/get)

  ```CPP
  template< std::size_t I, class... Types >
  constexpr std::variant_alternative_t<I, std::variant<Types...>>&
      get( std::variant<Types...>& v );
  template< std::size_t I, class... Types >
  constexpr std::variant_alternative_t<I, std::variant<Types...>>&&
      get( std::variant<Types...>&& v );
  template< std::size_t I, class... Types >
  constexpr const std::variant_alternative_t<I, std::variant<Types...>>&
      get( const std::variant<Types...>& v );
  template< std::size_t I, class... Types >
  constexpr const std::variant_alternative_t<I, std::variant<Types...>>&&
      get( const std::variant<Types...>&& v );
  ```

  基于索引地获取`variant`的值，如果`v`存储的就是第`I`的类型的值，那么返回这个值的引用，否则，抛出`std::bad_variant_access`.如果`I`超出了`variant`定义的类型总数，程序错误。

  ```CPP
  template< class T, class... Types >
  constexpr T& get( std::variant<Types...>& v );
  template< class T, class... Types >
  constexpr T&& get( std::variant<Types...>&& v );
  template< class T, class... Types >
  constexpr const T& get( const std::variant<Types...>& v );
  template< class T, class... Types >
  constexpr const T&& get( const std::variant<Types...>&& v );
  ```

  基于类型获取`variant`的值，如果`v`存储的就是`T`类型的值，那么返回这个值的引用，否则，抛出`std::bad_variant_access`.如果`T`不在`variant`定义的类型里，程序错误。

* [get_if](https://en.cppreference.com/w/cpp/utility/variant/get_if)

  ```CPP
  template< std::size_t I, class... Types >
  constexpr std::add_pointer_t<std::variant_alternative_t<I, std::variant<Types...>>>
      get_if( std::variant<Types...>* pv ) noexcept;
  template< std::size_t I, class... Types >
  constexpr std::add_pointer_t<const std::variant_alternative_t<I, std::variant<Types...>>>
      get_if( const std::variant<Types...>* pv ) noexcept;
  ```

  不抛出异常的版本，如果`pv->index() == I`，那么返回指向这个值的指针，否则，返回空指针。如果`I`超出了`variant`定义的类型总数，程序错误。

  ```CPP
  template< class T, class... Types >
  constexpr std::add_pointer_t<T>
      get_if( std::variant<Types...>* pv ) noexcept;
  template< class T, class... Types >
  constexpr std::add_pointer_t<const T>
      get_if( const std::variant<Types...>* pv ) noexcept;
  ```

  不抛出异常的版本，如果`pv`存储的就是`T`类型的值，返回指向这个值的指针，否则，返回空指针。如果`T`不在`variant`定义的类型里，程序错误。

* [holds_alternative](https://en.cppreference.com/w/cpp/utility/variant/holds_alternative)

  ```CPP
  template< class T, class... Types >
  constexpr bool holds_alternative( const std::variant<Types...>& v ) noexcept;
  ```

  检查`v`当前存储的值是否是类型`T`,如果`T`不在`variant`定义的类型里，程序错误。

## 帮助类

* [monostate](https://en.cppreference.com/w/cpp/utility/variant/monostate)

  ```CPP
  struct monostate { };
  ```

  用在`variant`的第一个模板实参上，那么`variant`的默认构造函数便是存储`monostate`.
