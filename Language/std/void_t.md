# void_t

在头文件`<type_traits>`中定义，就是`void`类型，用于模板元编程利用`SFINAE`。

参考文档

* CPP Reference[void_t](https://en.cppreference.com/w/cpp/types/void_t)

## 声明

```CPP
template< class... >
using void_t = void;
```

利用了`C++17`的`using`模板。

## 常用用途

通常用来检测模板类型的合法性，或者是表达式类型的合法性。

```CPP
// primary template handles types that have no nested ::type member:
template<class, class = void>
struct has_type_member : std::false_type {};
 
// specialization recognizes types that do have a nested ::type member:
template<class T>
struct has_type_member<T, std::void_t<typename T::type>> : std::true_type {};
```

可见，如果模板实参存在类内类型定义为`type`，结构体`has_type_member`就会有`::value`为真。
