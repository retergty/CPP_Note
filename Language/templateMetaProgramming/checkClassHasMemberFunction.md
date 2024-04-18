# 检查提供的模板实参是否含有成员函数

```CPP
#include <type_traits>


template<class T, class A0>
static auto test_stream(int)
    -> decltype(std::declval<T>().stream(std::declval<A0>()),std::true_type{});
template<class, class A0>
static auto test_stream(long) -> std::false_type;

template<class T, class Arg>
struct has_stream : decltype(test_stream<T, Arg>(0)){};
```

这个使用了`SFINAE`以及`decltype`实现了对于类是否含有成员函数`stream`使用不同的重载函数`test_stream`，这样之后类`has_stream`就会有不同的基类，只需要使用`has_stream<T,Arg>::value`就会有一个布尔值表示是否有成员函数`stream`.

利用`C++14`的变量模板，我们可以更加简便地写出。

```CPP
template<typename T,typename D = int>
constexpr bool has_clear = false;

template<typename T>
constexpr bool has_clear<T, decltype(std::declval<T>().clear(),int())> = true;
```

注意！默认实参是必须的，因为模板参数推导发生在主模板间，而不是偏特化，只有指定了默认实参，我们才可以形如`has_clear<std::vector<double>>`使用，调用流程是先推导出实参`T=std::vector<double>`,`D=int`，进行偏特化实参替换，有`clear`方法，偏特化为`has_clear<T,int>`使用这个偏特化。没有的话，根据`SFINAE`，剔除偏特化。

不能如下定义

```CPP
template<typename T,typename D = int>
constexpr bool has_clear = false;

template<typename T>
constexpr bool has_clear<T, int> = decltype(std::declval<T>().clear(),std::true_type())::value;
```

原因还是`SFINAE`不在值处发生作用。
