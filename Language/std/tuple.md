# tuple

定义在头文件`<tuple>`

`std::tuple`可以保存多个不同类型的成员变量，是一个固定大小的不同类型值的集合,是范化的`std::pair`.如果`std::is_trivially_destructible<Ti>::value`为真，则`std::tuple`也是平凡可析构的.

参考文档

* [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple.html)

## 类原型

```CPP
template< class... Types >
class tuple;
```

`Types...`是`tuple`可以存储的类型以及顺序.

## 非成员函数

### 创建tuple
  
* [make_tuple](https://en.cppreference.com/w/cpp/utility/tuple/make_tuple.html)

  ```CPP
  template< class... Types >
  std::tuple<VTypes...> make_tuple( Types&&... args );
  ```

  创建一个元组对象.根据传入的参数类型决定元组类型.

  对于`Types`中的`Ti`，对应的`Vi`类型为`std::decay<Ti>::type`，除非`decay`的结果是`std::reference_wrapper<X>`，那么结果就是`X&`.

  ```CPP
  // heterogeneous tuple construction
      int n = 1;
      auto t = std::make_tuple(10, "Test", 3.14, std::ref(n), n);
      n = 7;
      std::cout << "The value of t is ("
                << std::get<0>(t) << ", "
                << std::get<1>(t) << ", "
                << std::get<2>(t) << ", "
                << std::get<3>(t) << ", "
                << std::get<4>(t) << ")\n";
  ```

  结果是

  ```text
  The value of t is (10, Test, 3.14, 7, 1)
  ```

  可见`std::ref`包装下`tuple`的对应类型就是`int&`.

* [forward_as_tuple](https://en.cppreference.com/w/cpp/utility/tuple/forward_as_tuple.html)

  ```CPP
  template< class... Types >
  std::tuple<Types&&...> forward_as_tuple( Types&&... args ) noexcept;
  ```

  类似于`std::tuple<Types&&...>(std::forward<Types>(args)...)`.

### 级联tuple

* [tuple_cat](https://en.cppreference.com/w/cpp/utility/tuple/tuple_cat.html)

  ```CPP
  template< tuple-like... Tuples >
  constexpr std::tuple</* CTypes */...> tuple_cat( Tuples&&... args );
  ```

  将这些`tuple`级联起来，返回级联结果.

### 解包tuple

* [tie](https://en.cppreference.com/w/cpp/utility/tuple/tie.html)

  ```CPP
  template< class... Types >
  std::tuple<Types&...> tie( Types&... args ) noexcept;
  ```

  创建一个`arg`参数的左值引用的`tuple`.也可以是`std::ignore`.

  `std::tie`可以用来解包`tuple`或者是`pair`(因为`std::tie`有从`pair`的赋值构造函数)

  ```CPP
  bool result;
  std::tie(std::ignore, result) = set.insert(value);

  // Implicit conversions are permitted:
  std::tuple<char, short> coordinates(6, 9);
  std::tie(x, y) = coordinates;
  ```

### 获取tuple元素

* [get](https://en.cppreference.com/w/cpp/utility/tuple/get.html)

  ```CPP
  template< std::size_t I, class... Types >
  typename std::tuple_element<I, std::tuple<Types...>>::type&
      get( std::tuple<Types...>& t ) noexcept;

  template< std::size_t I, class... Types >
  typename std::tuple_element<I, std::tuple<Types...>>::type&&
      get( std::tuple<Types...>&& t ) noexcept;

  template< std::size_t I, class... Types >
  const typename std::tuple_element<I, std::tuple<Types...>>::type&
      get( const std::tuple<Types...>& t ) noexcept;

  template< std::size_t I, class... Types >
  const typename std::tuple_element<I, std::tuple<Types...>>::type&&
      get( const std::tuple<Types...>&& t ) noexcept;

  template< class T, class... Types >
  constexpr T& get( std::tuple<Types...>& t ) noexcept;

  template< class T, class... Types >
  constexpr T&& get( std::tuple<Types...>&& t ) noexcept;

  template< class T, class... Types >
  constexpr const T& get( const std::tuple<Types...>& t ) noexcept;

  template< class T, class... Types >
  constexpr const T&& get( const std::tuple<Types...>&& t ) noexcept;
  ```

  (1)到(4)是按照下标获取`tuple`的元素，`I`必须是整型`[0,sizeof...(Types))`

  (5)到(8)是按照类型值获取元素，如果有多个相同类型，那么编译失败.

  ```CPP
  auto x = std::make_tuple(1, "Foo", 3.14);
  // Index-based access
  std::cout << "( " << std::get<0>(x)
            << ", " << std::get<1>(x)
            << ", " << std::get<2>(x)
            << " )\n";
  
  // Type-based access (since C++14)
  std::cout << "( " << std::get<int>(x)
            << ", " << std::get<const char*>(x)
            << ", " << std::get<double>(x)
            << " )\n";
  ```

## 帮助类

* [tuple_size](https://en.cppreference.com/w/cpp/utility/tuple/tuple_size.html)

  ```CPP
  template< class... Types >
  struct tuple_size< std::tuple<Types...> >
      : std::integral_constant<std::size_t, sizeof...(Types)> { };
  ```

  编译期获取`tuple`的成员数量.

  ```CPP
  template< class T >
  constexpr std::size_t tuple_size_v = tuple_size<T>::value;
  ```

* [tuple_element](https://en.cppreference.com/w/cpp/utility/tuple/tuple_element.html)

  ```CPP
  template< std::size_t I, class... Types >
  struct tuple_element< I, std::tuple<Types...> >;
  ```

  提供对元组元素类型的编译时索引访问
