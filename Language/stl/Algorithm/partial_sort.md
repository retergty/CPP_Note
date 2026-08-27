# partial_sort

定义在头文件`algorithm`中，对指定迭代器范围内的前一部分元素排序，使其包含整个范围中最小的元素。

参考文档

* [std::partial_sort](https://en.cppreference.com/w/cpp/algorithm/partial_sort.html)

## 函数原型

```CPP
template< class RandomIt >
void partial_sort( RandomIt first, RandomIt middle, RandomIt last );

template< class ExecutionPolicy, class RandomIt >
void partial_sort( ExecutionPolicy&& policy,
                   RandomIt first, RandomIt middle, RandomIt last );

template< class RandomIt, class Compare >
void partial_sort( RandomIt first, RandomIt middle,
                   RandomIt last, Compare comp );

template< class ExecutionPolicy, class RandomIt, class Compare >
void partial_sort( ExecutionPolicy&& policy,
                   RandomIt first, RandomIt middle,
                   RandomIt last, Compare comp );
```

* 1）对元素使用`operator<`进行比较。
* 3）使用用户定义的比较器函数`comp`，注意，必须要是严格弱序。
* 2）和4）是对应的并行执行策略重载。

函数执行完毕后，范围`[first,middle)`包含原范围中最小的`middle-first`个元素，并且这些元素已经按照非降序排列。

范围`[middle,last)`内元素的顺序未指定，但其中任意元素都不会排在`[first,middle)`内的元素之前。如果`middle == first`，则不对范围进行排序。

`first`、`middle`和`last`必须属于同一个有效范围，并且`middle`必须位于`[first,last]`内。迭代器类型必须满足随机访问迭代器的要求。

比较器函数原型为

```CPP
bool comp(const Type1& a, const Type2& b);
```

注意，严格弱序指的是比较操作必须满足

* 非自反性:`comp(a,a)`必须为`false`
* 非对称性:若`comp(a,b)`为真，则`comp(b,a)`为假
* 传递性:若`comp(a,b)`,`comp(b,c)`为真，则`comp(a,c)`也为真
* 等价的传递性:若`!comp(a,b) && !comp(b,a)`为真，那么`a,b`等价，这个性质可以传递

千万不能用`<=`号在比较器上。

## 描述

`partial_sort`通常使用堆排序实现：

* 首先将`[first,middle)`构造成最大堆。
* 遍历`[middle,last)`，如果当前元素小于堆顶，则用当前元素替换堆顶，并重新调整堆。
* 最后对堆进行排序，得到有序的`[first,middle)`。

设整个范围的元素数量为`N`，需要排序的元素数量为`M`，其比较次数约为`N·log(M)`。当只需要最小的少量元素时，`partial_sort`通常比对整个范围调用`sort`更高效。

## 例子

```CPP
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>

int main()
{
    std::array<int, 10> s{5, 7, 4, 2, 8, 6, 1, 9, 0, 3};

    std::partial_sort(s.begin(), s.begin() + 4, s.end());

    std::cout << "the four smallest elements: ";
    for (auto it = s.begin(); it != s.begin() + 4; ++it)
        std::cout << *it << ' ';
    std::cout << '\n';

    std::partial_sort(s.begin(), s.begin() + 3, s.end(),
                      std::greater<int>());

    std::cout << "the three largest elements: ";
    for (auto it = s.begin(); it != s.begin() + 3; ++it)
        std::cout << *it << ' ';
    std::cout << '\n';
}
```