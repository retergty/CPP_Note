# iterator traits

定义在头文件`iterator`中,`iterator_traits`是一个类，为所有迭代器提供一个统一的接口。这使得我们可以仅对迭代器设置算法。

参考文档

* CPP Reference[iterator_traits](https://en.cppreference.com/w/cpp/iterator/iterator_traits)

## 声明

```CPP
template< class Iter >
struct iterator_traits;
template< class T >
struct iterator_traits<T*>;
template< class T >
struct iterator_traits<const T*>;
```

## 类内类型定义

* `difference_type`定义为`Iter::difference_type`
* `value_type`定义为`Iter::value_type`
* `pointer`定义为`Iter::pointer`
* `reference`定义为`Iter::reference`
* `iterator_category`定义为`Iter::iterator_category`

如果迭代器没有上述的类型中的任意一个，那么`iterator_traits`不存在这些类型定义，也就是说可以使用`SFINAE`。

## 例子

```CPP
#include <iostream>
#include <iterator>
#include <list>
#include <vector>
 
template<class BidirIt>
void my_reverse(BidirIt first, BidirIt last)
{
    typename std::iterator_traits<BidirIt>::difference_type n = std::distance(first, last);
    for (--n; n > 0; n -= 2)
    {
        typename std::iterator_traits<BidirIt>::value_type tmp = *first;
        *first++ = *--last;
        *last = tmp;
    }
}
 
int main()
{
    std::vector<int> v{1, 2, 3, 4, 5};
    my_reverse(v.begin(), v.end());
    for (int n : v)
        std::cout << n << ' ';
    std::cout << '\n';
 
    std::list<int> l{1, 2, 3, 4, 5};
    my_reverse(l.begin(), l.end());
    for (int n : l)
        std::cout << n << ' ';
    std::cout << '\n';
 
    int a[]{1, 2, 3, 4, 5};
    my_reverse(a, a + std::size(a));
    for (int n : a)
        std::cout << n << ' ';
    std::cout << '\n';
 
//  std::istreambuf_iterator<char> i1(std::cin), i2;
//  my_reverse(i1, i2); // compilation error: i1, i2 are input iterators
}
```
