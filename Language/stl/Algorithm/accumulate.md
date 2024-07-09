# accumulate

定义在头文件`<numeric>`中，在给定迭代器范围内实现累加。

参考文档

* [accumulate](https://en.cppreference.com/w/cpp/algorithm/accumulate)

## 声明

```CPP
template< class InputIt, class T >
T accumulate( InputIt first, InputIt last, T init );
template< class InputIt, class T, class BinaryOp >
T accumulate( InputIt first, InputIt last, T init, BinaryOp op );
```

计算从`[first, last)`范围，初始值是`init`的和。

`1`初始化类型`T`的累加器`acc`为`init`,并按顺序对范围内的元素迭代器`i`使用`acc = acc + *i`.

`2`初始化类型`T`的累加器`acc`为`init`,并按顺序对范围内的元素迭代器`i`使用`acc = op(acc, *i)`.

## 可能实现

```CPP
template<class InputIt, class T>
constexpr // since C++20
T accumulate(InputIt first, InputIt last, T init)
{
    for (; first != last; ++first)
        init = std::move(init) + *first; // std::move since C++20
 
    return init;
}

template<class InputIt, class T, class BinaryOperation>
constexpr // since C++20
T accumulate(InputIt first, InputIt last, T init, BinaryOperation op)
{
    for (; first != last; ++first)
        init = op(std::move(init), *first); // std::move since C++20
 
    return init;
}
```

## 例子

```CPP
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
 
int main()
{
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
 
    int sum = std::accumulate(v.begin(), v.end(), 0);
    int product = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>());
 
    auto dash_fold = [](std::string a, int b)
    {
        return std::move(a) + '-' + std::to_string(b);
    };
 
    std::string s = std::accumulate(std::next(v.begin()), v.end(),
                                    std::to_string(v[0]), // start with first element
                                    dash_fold);
 
    // Right fold using reverse iterators
    std::string rs = std::accumulate(std::next(v.rbegin()), v.rend(),
                                     std::to_string(v.back()), // start with last element
                                     dash_fold);
 
    std::cout << "sum: " << sum << '\n'
              << "product: " << product << '\n'
              << "dash-separated string: " << s << '\n'
              << "dash-separated string (right-folded): " << rs << '\n';
}
```