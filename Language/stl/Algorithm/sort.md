# sort

定义在头文件`algorithm`中，在指定的迭代器范围内排序元素，非降序排序（从小到大），不保证相等元素的顺序.

参考文档

* [std::sort](https://en.cppreference.com/w/cpp/algorithm/sort.html)

## 函数原型

```CPP
template< class RandomIt >
void sort( RandomIt first, RandomIt last );

template< class ExecutionPolicy, class RandomIt >
void sort( ExecutionPolicy&& policy,
           RandomIt first, RandomIt last );

template< class RandomIt, class Compare >
void sort( RandomIt first, RandomIt last, Compare comp );

template< class ExecutionPolicy, class RandomIt, class Compare >
void sort( ExecutionPolicy&& policy,
           RandomIt first, RandomIt last, Compare comp );

```

* 1）使用`operator<`比较元素
* 3）使用用户定义的比较器函数`comp`，注意，必须要是严格弱序.

比较器函数原型为

```CPP
bool comp(const Type1& a, const Type2& b);
```

注意，严格弱序指的是比较操作必须满足

* 非自反性:`comp(a,a)`必须为`false`
* 非对称性:若`comp(a,b)`为真，则`comp(b,a)`为假
* 传递性:若`comp(a,b)`,`comp(b,c)`为真，则`comp(a,c)`也为真
* 等价的传递性:若`!comp(a, b) && !comp(b, a)`为真，那么`a,b`等价，这个性质可以传递。

千万不能用`<=`号在比较器上.

## 例子

```CPP
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string_view>

int main()
{
    std::array<int, 10> s{5, 7, 4, 2, 8, 6, 1, 9, 0, 3};
 
    auto print = [&s](std::string_view const rem)
    {
        for (auto a : s)
            std::cout << a << ' ';
        std::cout << ": " << rem << '\n';
    };
 
    std::sort(s.begin(), s.end());
    print("sorted with the default operator<");
 
    std::sort(s.begin(), s.end(), std::greater<int>());
    print("sorted with the standard library compare function object");
 
    struct
    {
        bool operator()(int a, int b) const { return a < b; }
    }
    customLess;
 
    std::sort(s.begin(), s.end(), customLess);
    print("sorted with a custom function object");
 
    std::sort(s.begin(), s.end(), [](int a, int b)
                                  {
                                      return a > b;
                                  });
    print("sorted with a lambda expression");
}
```