# declval

在头文件`utility`定义，用于把类型`T`变为右值引用，这样我们就可以在`decltype`中使用成员函数了。

## 声明

```CPP
template< class T >
typename std::add_rvalue_reference<T>::type declval() noexcept;
```

## 可能实现

```CPP
template<typename T>
typename std::add_rvalue_reference<T>::type declval() noexcept
{
    static_assert(false, "declval not allowed in an evaluated context");
}
```

## 用途

把类型`T`变为右值引用，允许我们在`decltype`中使用成员函数而不需要访问构造函数了，只能用在不处理表达式值的语句中，比如`decltype`.否则就会报错

```CPP
#include <iostream>
#include <utility>
 
struct Default
{
    int foo() const { return 1; }
};
 
struct NonDefault
{
    NonDefault() = delete;
    int foo() const { return 1; }
};
 
int main()
{
    decltype(Default().foo()) n1 = 1;                   // type of n1 is int
//  decltype(NonDefault().foo()) n2 = n1;               // error: no default constructor
    decltype(std::declval<NonDefault>().foo()) n2 = n1; // type of n2 is int
    std::cout << "n1 = " << n1 << '\n'
              << "n2 = " << n2 << '\n';
}
```

比如没有默认构造函数，或者不知道模板参数是否有构造函数，使用`declval`就可以不必访问构造函数而直接访问成员函数。当然，此时不能计算表达式的值，否则就会出现错误。
