# numeric_limits

在头文件`limits`中定义，使用模板类来获得模板实参数值的信息，比如最大值，最小值等.

参考文档

* [CPP Reference](https://en.cppreference.com/w/cpp/types/numeric_limits)

## 类原型

```CPP
template< class T > class numeric_limits;
```

* `T`指定的数值类型

### 模板特化

`C++`标准模板特化了以下的类型

```CPP
template<> class numeric_limits<bool>;
template<> class numeric_limits<char>;
template<> class numeric_limits<signed char>;
template<> class numeric_limits<unsigned char>;
template<> class numeric_limits<wchar_t>;
template<> class numeric_limits<char8_t>;
template<> class numeric_limits<char16_t>;
template<> class numeric_limits<char32_t>;
template<> class numeric_limits<short>;
template<> class numeric_limits<unsigned short>;
template<> class numeric_limits<int>;
template<> class numeric_limits<unsigned int>;
template<> class numeric_limits<long>;
template<> class numeric_limits<unsigned long>;
template<> class numeric_limits<long long>;
template<> class numeric_limits<unsigned long long>;
template<> class numeric_limits<float>;
template<> class numeric_limits<double>;
template<> class numeric_limits<long double>;
```

所有的`cv`限定符都与无`cv`限定符的版本相同.比如`const int`和`int`相同.

## 常见成员函数

### 获取信息

* [min](https://en.cppreference.com/w/cpp/types/numeric_limits/min)

  ```CPP
  static constexpr T min() noexcept;
  ```

  获取正规情况下的最小值

* [lowest](https://en.cppreference.com/w/cpp/types/numeric_limits/lowest)

  ```CPP
  static constexpr T lowest() noexcept;
  ```

  获取任何时候的最小值

* [max](https://en.cppreference.com/w/cpp/types/numeric_limits/max)

  ```CPP
  static constexpr T max() noexcept;
  ```

  获取最大值.
