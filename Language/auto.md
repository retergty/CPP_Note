# auto

参考文档

* CPP reference[Placeholder type specifiers](https://en.cppreference.com/w/cpp/language/auto)
* MSVC 参考手册[auto](https://learn.microsoft.com/en-us/cpp/cpp/auto-cpp?view=msvc-170)
* csdn[auto关键字](https://blog.csdn.net/qq_58665528/article/details/122575383)

`auto`指示编译器进行类型推断，编译器推断的类型由上下文和`auto`关键字决定。

## 语法

`auto`可以用于许多地方。

* `auto x = expr`形式，`x`的类型由`expr`推断得来，推导规则和模板参数推导规则相同。比如对于`const auto& i = expr;`，`i`的类型就和模板`template<class U> void f(const U& u)`实例化`f(expr)`时`U`的类型。

## 例子

```CPP
int i = 0;
const int ci = 0;
int* pi = &i;
const int* cpi = &ci;
int* const pic = &i;
int& ii = i;
const int& cii = i;
 
auto p = i;    //int
auto p2 = ci;  //int，因为ci有顶层const
auto p3 = pi;  //int*
auto p4 = cpi; //const int*, 因为是cpi底层const，不用去除
auto p5 = pic; //int*, 因为是pic顶层const
auto p6 = ii;  //int, 因为ii有引用
auto p7 = cii; //int, 因为cii去除引用后，const int为顶层const
```

```CPP
int i = 0;
const int ci = 0;
int* pi = &i;
const int* cpi = &ci;
int* const pic = &i;
int& ii = i;
const int& cii = i;
 
auto* p = &i;   //int*
auto* p2 = &ci; //const int*，因为&ci相当于const int*，是底层const
auto* p3 = pi;  //int*
auto* p4 = cpi; //const int*, 因为是cpi底层const，不用去除
auto* p5 = pic; //int*, 因为是pic顶层const
auto* p6 = &ii; //int*, 因为&ii有引用
auto* p7 = &cii;//const int*, 因为&cii去除引用后，const int*为低层const
```

```CPP
int i = 0;
const int ci = 0;
int* pi = &i;
const int* cpi = &ci;
int* const pic = &i;
int& ii = i;
const int& cii = i;
 
auto& p = i;   //int&
auto& p2 = ci; //const int&
auto& p3 = pi; //int* &
auto& p4 = cpi;//const int* &
auto& p5 = pic;//int* const &
auto& p6 = ii; //int&
auto& p7 = cii;//const int&
```
