# 检查提供的模板函数是否有类内定义类型

```CPP
// primary template handles types that have no nested ::type member:
template<class, class = void>
struct has_type_member : std::false_type {};
 
// specialization recognizes types that do have a nested ::type member:
template<class T>
struct has_type_member<T, std::void_t<typename T::type>> : std::true_type {};
```

使用了`void_t`和`SFINAE`检查类型`T::type`是否合法。

可以使用`C++17`的变量模板更加简洁。

```CPP
template<typename T,typename D = void>
constexpr bool has_const_iterator = false;

template<typename T>
constexpr bool has_const_iterator<T, std::void_t<typename T::const_iterator>> = true;
```
