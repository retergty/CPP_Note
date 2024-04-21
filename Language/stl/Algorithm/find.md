# find find_if find_if_not

定义在头文件`algorithm`中，在指定的迭代器范围（左闭右开）中查找指定值或者是满足指定条件的对象。

参考文档

* CPP Reference[find](https://en.cppreference.com/w/cpp/algorithm/find)

## 声明

```CPP
template< class InputIt, class T >
InputIt find( InputIt first, InputIt last, const T& value );
template< class ExecutionPolicy, class ForwardIt, class T >
ForwardIt find( ExecutionPolicy&& policy,
                ForwardIt first, ForwardIt last, const T& value );

template< class InputIt, class UnaryPred >
InputIt find_if( InputIt first, InputIt last, UnaryPred p );
template< class ExecutionPolicy, class ForwardIt, class UnaryPred >
ForwardIt find_if( ExecutionPolicy&& policy,
                   ForwardIt first, ForwardIt last, UnaryPred p );

template< class InputIt, class UnaryPred >
InputIt find_if_not( InputIt first, InputIt last, UnaryPred q );
template< class ExecutionPolicy, class ForwardIt, class UnaryPred >
ForwardIt find_if_not( ExecutionPolicy&& policy,
                       ForwardIt first, ForwardIt last, UnaryPred q );
```

返回在指定迭代器（左闭右开）中匹配指定要求的对象的迭代器，如果没有找到，则返回`last`.

`p`和`q`是一个一元谓词函数，接受迭代器指向的值，且不应该修改指向的值，`p`对满足条件的对象返回`true`，`q`对满足条件的对象返回`false`.

## 可能实现

```CPP
template<class InputIt, class T>
constexpr InputIt find(InputIt first, InputIt last, const T& value)
{
    for (; first != last; ++first)
        if (*first == value)
            return first;
 
    return last;
}
template<class InputIt, class UnaryPred>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPred p)
{
    for (; first != last; ++first)
        if (p(*first))
            return first;
 
    return last;
}
template<class InputIt, class UnaryPred>
constexpr InputIt find_if_not(InputIt first, InputIt last, UnaryPred q)
{
    for (; first != last; ++first)
        if (!q(*first))
            return first;
 
    return last;
}
```

## 例子

```CPP
#include <algorithm>
#include <array>
#include <iostream>
 
int main()
{
    const auto v = {1, 2, 3, 4};
 
    for (const int n : {3, 5})
        (std::find(v.begin(), v.end(), n) == std::end(v))
            ? std::cout << "v does not contain " << n << '\n'
            : std::cout << "v contains " << n << '\n';
 
    auto is_even = [](int i) { return i % 2 == 0; };
 
    for (auto const& w : {std::array{3, 1, 4}, {1, 3, 5}})
        if (auto it = std::find_if(begin(w), end(w), is_even); it != std::end(w))
            std::cout << "w contains an even number " << *it << '\n';
        else
            std::cout << "w does not contain even numbers\n";
}
```
