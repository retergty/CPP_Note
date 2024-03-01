# initializer_list

在头文件`initializer_list`中定义，类型为`std::initializer_list<T>`的对象是一个轻量级代理对象，它提供成员为`const T`类型对象的数组的访问(这个数组叫做`backing array`)。（这个数组有可能是分配到只读存储区的）

参考文档

* cpp reference[std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)
* MSVC 参考文档[initializer_list 类](https://learn.microsoft.com/zh-cn/cpp/standard-library/initializer-list-class?view=msvc-170)
* MSVC 参考文档[大括号初始化](https://learn.microsoft.com/zh-cn/cpp/cpp/initializing-classes-and-structs-without-constructors-cpp?view=msvc-170)

一个对应类型的`std::initializer_list`会在如下的情况下构造：

* 大括号初始化来初始化一个对象，且这个对象有对应的接受`std::initializer_list`的构造函数
* 大括号初始化用于等号的右边或者是函数调用参数时，对应的赋值运算符/函数调用接受 `std::initializer_list`参数。
* 大括号初始化绑定在`auto`上，包括范围`for`.

`std::initializer_list`的实现方法可能是包含一对指针，分别指向大括号初始化的第一个元素和最后一个元素的后面。

`std::initializer_list`并不会复制对应的数组，而是存储指针。

## 例子

```CPP
#include <initializer_list>
#include <iostream>
#include <vector>
 
template<class T>
struct S
{
    std::vector<T> v;
 
    S(std::initializer_list<T> l) : v(l)
    {
         std::cout << "constructed with a " << l.size() << "-element list\n";
    }
 
    void append(std::initializer_list<T> l)
    {
        v.insert(v.end(), l.begin(), l.end());
    }
 
    std::pair<const T*, std::size_t> c_arr() const
    {
        return {&v[0], v.size()}; // copy list-initialization in return statement
                                  // this is NOT a use of std::initializer_list
    }
};
 
template<typename T>
void templated_fn(T) {}
 
int main()
{
    S<int> s = {1, 2, 3, 4, 5}; // copy list-initialization
    s.append({6, 7, 8});        // list-initialization in function call
 
    std::cout << "The vector now has " << s.c_arr().second << " ints:\n";
 
    for (auto n : s.v)
        std::cout << n << ' ';
    std::cout << '\n';
 
    std::cout << "Range-for over brace-init-list: \n";
 
    for (int x : {-1, -2, -3}) // the rule for auto makes this ranged-for work
        std::cout << x << ' ';
    std::cout << '\n';
 
    auto al = {10, 11, 12}; // special rule for auto
 
    std::cout << "The list bound to auto has size() = " << al.size() << '\n';
 
//  templated_fn({1, 2, 3}); // compiler error! "{1, 2, 3}" is not an expression,
                             // it has no type, and so T cannot be deduced
    templated_fn<std::initializer_list<int>>({1, 2, 3}); // OK
    templated_fn<std::vector<int>>({1, 2, 3});           // also OK
}
```

输出为

```text
constructed with a 5-element list
The vector now has 8 ints:
1 2 3 4 5 6 7 8
Range-for over brace-init-list: 
-1 -2 -3 
The list bound to auto has size() = 3
```

可以在大括号里加上大括号，表示使用几个元素构造`initializer_list`接受的对象类型。

```CPP
map<int, string> m1{ {1, "a"}, {2, "b"} };
```

首先，`backing array`里的每一项都是根据大括号里的项，复制初始化出来的，比如第一项，调用复制构造函数`{1,"a"}`初始化第一项，第二项同理。之后再根据`backing array`初始化`std::initializer_list`.
