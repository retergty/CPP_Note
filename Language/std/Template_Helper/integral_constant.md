# integral_constant

在头文件`<type_traits>`中定义。这个类包装了指定的常量。

参考文档

* CPP Reference[integral_constant](https://en.cppreference.com/w/cpp/types/integral_constant)

## 声明

```CPP
template< class T, T v >
struct integral_constant;
```

## 简便别名模板

一个简便的别名模板`std::bool_constant`定义来用于当`T`是布尔值时。

```CPP
template< bool B >
using bool_constant = integral_constant<bool, B>;
```

## 模板特化

以下有两个模板全特化。

```CPP
using true_type = std::integral_constant<bool, true>;
using false_type = std::integral_constant<bool, false>
```

## 类内类型定义

在这个类中定义的类型如下

```CPP
using value_type = T;
using type = integral_constant;
```

## 类内常量

```CPP
static constexpr T value = v;
```

## 可能的实现

```CPP
template<class T, T v>
struct integral_constant
{
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant; // using injected-class-name
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; } // since c++14
};
```
