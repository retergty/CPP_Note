# enable_if()

在头文件`type_traits`中定义，作为模板元编程重要的工具，能够实现编译期函数分发等复杂操作。

参考文档

* CPP参考手册[enabble_if](https://en.cppreference.com/w/cpp/types/enable_if)
* MSVC参考手册[enable_if](https://learn.microsoft.com/en-us/cpp/standard-library/enable-if-class?view=msvc-170)

## 声明

```CPP
template< bool B, class T = void >
struct enable_if;
```

如果`B`为`true`，则`enable_if`有一个公有成员`typedef`为`T`.如果`B`为`false`，则没有任何成员`typedef`。

这个模板类通过著名的SFINAE来把函数移除重载集合，允许按照不同的类型进行不同的函数或者模板特化。

这个类还有一个帮助类型

```CPP
template< bool B, class T = void >
using enable_if_t = typename enable_if<B,T>::type;
```

## 可能的实现方式

```CPP
template<bool B, class T = void>
struct enable_if {};
 
template<class T>
struct enable_if<true, T> { typedef T type; };
```

`enable_if()`本身是利用模板偏特化实现的，当`B=ture`时，选择偏特化的模板`enable_if<true, T>`,存在`typedef T type`,成功，否则没有。

## 例子

```CPP
struct T
{
    enum { int_t, float_t } type;
 
    template<typename Integer,
             std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
    T(Integer) : type(int_t) {}
 
    template<typename Floating,
             std::enable_if_t<std::is_floating_point<Floating>::value, bool> = true>
    T(Floating) : type(float_t) {} // OK
};
```

使用`enable_if()`实现了对模板参数是整数还是浮点数进行了分发构造函数。

## 不要滥用`enable_if`

滥用`enable_if()`会带来极大的阅读难度，以及调试难度，MSVC文档提供了三个推荐。

* 不要使用`enable_if()`来在编译期选择实现，比如写下`enable_if`对于条件`CONDITION`和条件`!CONDITION`.
* 不要使用`enable_if()`强化使用需求，要是想要检测模板实参，检测失败报错，使用`static_assert`
* 使用`enable_if()`来使得重载集更加清晰。
