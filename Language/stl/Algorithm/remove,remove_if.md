# remove,remove_if

定义在头文件`algorithm`中，在指定的迭代器范围内删除满足条件的项

参考文档

* [std::remove, std::remove_if](https://en.cppreference.com/w/cpp/algorithm/remove)

## 声明

```CPP
template< class ForwardIt, class T >
ForwardIt remove( ForwardIt first, ForwardIt last, const T& value );

template< class ExecutionPolicy, class ForwardIt, class T >
ForwardIt remove( ExecutionPolicy&& policy,
                  ForwardIt first, ForwardIt last, const T& value );

template< class ForwardIt, class UnaryPred >
ForwardIt remove_if( ForwardIt first, ForwardIt last, UnaryPred p );

template< class ExecutionPolicy, class ForwardIt, class UnaryPred >
ForwardIt remove_if( ExecutionPolicy&& policy,
                     ForwardIt first, ForwardIt last, UnaryPred p );
```

在指定的迭代器范围`[first,last)`内删除满足条件的项,并返回指向删除后的尾后迭代器，比如如果没有删除元素，则返回`last`.

`remove`删除与`value`相等的元素。

`remove_if`删除`p`返回为真时的元素。

如果`*first`的类型不是移动可赋值的，代码行为未定义。

## 描述

删除是通过移动范围内的元素实现的，使得不被删除的元素出现在容器的开头。

* 移动元素是通过移动赋值实现的。
* 移动操作是稳定的，没有被删除的元素保持原来的顺序。
* 底层容器实际上并没有缩短，假设代码返回`result`,那么`[result,last)`内的元素仍然存在，是可解引用的，但处于一个未指定的状态，因为移动赋值的关系。

由于`remove/remove_if`实际上并没有删除底层容器的元素，通常还需要调用容器的`erase`成员函数。

注意,`remove/remove_if`不能用在关联容器中，比如`std::set`或`std::map`因为它们迭代器解引用到的类型不是移动可赋值的，容器节点关键字不能被改变。

由于`remove`按照引用方式接受`value`，如果`value`是`[first,last)`的元素，可能会带来意想不到的的结果。相同的情况也可能发生在`remove_if`.

## 可能实现

```CPP
template<class ForwardIt, class T = typename std::iterator_traits<ForwardIt>::value_type>
ForwardIt remove(ForwardIt first, ForwardIt last, const T& value)
{
    first = std::find(first, last, value);
    if (first != last)
        for (ForwardIt i = first; ++i != last;)
            if (!(*i == value))
                *first++ = std::move(*i);
    return first;
}

template<class ForwardIt, class UnaryPred>
ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPred p)
{
    first = std::find_if(first, last, p);
    if (first != last)
        for (ForwardIt i = first; ++i != last;)
            if (!p(*i))
                *first++ = std::move(*i);
    return first;
}
```

## 例子

```CPP
#include <algorithm>
#include <cassert>
#include <cctype>
#include <complex>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
 
int main()
{
    std::string str1{"Text with some   spaces"};
 
    auto noSpaceEnd = std::remove(str1.begin(), str1.end(), ' ');
 
    // The spaces are removed from the string only logically.
    // Note, we use view, the original string is still not shrunk:
    std::cout << std::string_view(str1.begin(), noSpaceEnd) 
              << " size: " << str1.size() << '\n';
 
    str1.erase(noSpaceEnd, str1.end());
 
    // The spaces are removed from the string physically.
    std::cout << str1 << " size: " << str1.size() << '\n';
 
    std::string str2 = "Text\n with\tsome \t  whitespaces\n\n";
    str2.erase(std::remove_if(str2.begin(), 
                              str2.end(),
                              [](unsigned char x) { return std::isspace(x); }),
               str2.end());
    std::cout << str2 << '\n';
 
    std::vector<std::complex<double>> nums{{2, 2}, {1, 3}, {4, 8}};
    #ifdef __cpp_lib_algorithm_default_value_type
        nums.erase(std::remove(nums.begin(), nums.end(), {1, 3}), nums.end());
    #else
        nums.erase(std::remove(nums.begin(), nums.end(), std::complex<double>{1, 3}),
                   nums.end());
    #endif
    assert((nums == std::vector<std::complex<double>>{{2, 2}, {4, 8}}));
}
```
