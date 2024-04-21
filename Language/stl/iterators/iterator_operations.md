# iterator operation

本文总结了迭代器支持的操作。定义在头文件`iterator`中。

## distance

计算两个迭代器间的距离，会根据迭代器的类型自动选择高效的方法。

参考文档

* CPP Reference[distance](https://en.cppreference.com/w/cpp/iterator/distance)

### 声明

```CPP
template< class InputIt >
typename std::iterator_traits<InputIt>::difference_type
    distance( InputIt first, InputIt last );
```

返回的值满足

```text
first + distance = last
```

当然取决于迭代器的类型，这个式子可能需要转化为步进的循环结构。

### 例子

```CPP
#include <iostream>
#include <iterator>
#include <vector>
 
int main() 
{
    std::vector<int> v{3, 1, 4};
    std::cout << "distance(first, last) = "
              << std::distance(v.begin(), v.end()) << '\n'
              << "distance(last, first) = "
              << std::distance(v.end(), v.begin()) << '\n';
              // the behavior is undefined (until LWG940)
 
    static constexpr auto il = {3, 1, 4};
    // Since C++17 `distance` can be used in constexpr context.
    static_assert(std::distance(il.begin(), il.end()) == 3);
    static_assert(std::distance(il.end(), il.begin()) == -3);
}
```

## advance

将迭代器向前`n`步，会根据迭代器的类型自动选择高效的方法。

参考文档

* CPP Reference[advance](https://en.cppreference.com/w/cpp/iterator/advance)

### 声明

```CPP
template< class InputIt, class Distance >
constexpr void advance( InputIt& it, Distance n );
```

如果`n`是负数，而且`it`满足双向迭代器的需求，那么就会向后移动。

### 例子

```CPP
#include <iostream>
#include <iterator>
#include <vector>
 
int main() 
{
    std::vector<int> v{3, 1, 4};
 
    auto vi = v.begin();
    std::advance(vi, 2);
    std::cout << *vi << ' ';
 
    vi = v.end();
    std::advance(vi, -2);
    std::cout << *vi << '\n';
}
```
