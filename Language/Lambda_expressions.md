# Lambda expressions

参考文件

* [CPP reference Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda)

构造一个闭包(closure)，一个能够捕获作用域内变量的匿名函数对象。

## 语法

没有显式的模板参数列表

```text
[captures] front-attr(optional) (params) specs(optional) exception(optional)
back-attr(optional) trailing-type(optional) requires(optional) { body } 

[captures] { body }
```

具有显式的模板参数列表(C++20)

```text
[captures] <tparams> t-requires(optional) front-attr(optional) (params) specs(optional) exception(optional)
back-attr(optional) trailing-type(optional) requires(optional) { body } 

[captures] <tparams> t-requires(optional) { body }
```

* `captures`是一个逗号分隔的列表，捕获任意个变量，可以指定默认捕获行为(值捕获，引用捕获).
  `Lambda`表达式只能使用捕获到的外部变量，除非

  * 外部变量不是局部变量，或者是`static`的，或者具有线程生命周期。
  * 使用常量表达式初始化的引用。

  `Lambda`表达式只能读取捕获到的外部变量，除非

  * 外部变量是`const`且非`volatile`的整型，或者是枚举型，并使用了常量表达式初始化。
  * 外部变量是常量表达式`constexpr`且没有`mutable`的成员。

* `tparams`是一个非空的模板参数列表，用于给`template lambda`使用。

* `t-requires`给`tparams`加上限制

* `front-attr`匿名对象的`operator()`的限定修饰符

* `params`匿名对象`operator()`接受的参数列表。

* `specs`声明一系列修饰符，可以是`mutable`,`constexpr`,`consteval`,`static`

* `exception`声明异常,比如`noexcept`.

* `back-attr`匿名对象的`operator()`的限定修饰符

* `trailing-type`声明返回类型，格式是`-> retype`.

* `requires`匿名对象`operator()`的限制

* `body`匿名对象`operator()`的函数体。

## 泛型Lambda

如果在参数中使用`auto`或者声明了显式的模板参数（C++20），那么`Lambda`就变成了泛型。

## Lambda的类型

`Lambda`表达式的类型是纯右值(prvalue)，是一个唯一的未命名(unnamed)非聚合(non-union)非联合(non-aggregate)的类(class)类型,叫做闭包类型(closure type).在包含`Lambda`表达式的最小块作用域、类作用域或命名空间作用域中声明（出于 ADL 的目的）.

当且仅当 `captures`为空时，闭包类型才是结构(structural)的类型。

闭包类型具有以下成员，它们不能显式实例化、显式特例化或用在友元声明中

### `operator()(params)`

```CPP
ret operator()(params) { body }
template<template-params>
ret operator()(params) { body }
```

处理的函数体。当访问变量时，取决于捕获的类型决定是访问变量的副本(copy)还是访问变量的引用(reference)。

`parms`就是Lambda表达式中声明的`parms`,如果没有则为空。

`ret`的类型就是Lambda表达式中声明的`trailing-type`,如果没有声明，则自动推断返回值类型。

除非使用`mutable`修饰`Lambda`表达式，否则该函数都会是`const`的，值捕获的变量不能在函数体内修改。`operator()`不会是`virtual`的，不能有`volatile`修饰符。

`operator()`如果满足了`constexpr`函数的要求，那么`operator()`都是`constexpr`的。

如果在定义`Lambda`表达式时，参数使用了`auto`,那么就会依照顺序添加在`template-params`.

```CPP
// generic lambda, operator() is a template with two parameters
auto glambda = [](auto a, auto&& b) { return a < b; };
bool b = glambda(3, 3.14); // OK

// generic lambda, operator() is a template with one parameter
auto vglambda = [](auto printer)
{
    return [=](auto&&... ts) // generic lambda, ts is a parameter pack
    { 
      printer(std::forward<decltype(ts)>(ts)...);
        // nullary lambda (takes no parameters):
        return [=] { printer(ts...); };
    };
};
  
auto p = vglambda([](auto v1, auto v2, auto v3)
{
    std::cout << v1 << v2 << v3;
});

auto q = p(1, 'a', 3.14); // outputs 1a3.14
q();                      // outputs 1a3.14
```

注意悬垂引用(Dangling references)的问题，如果`Lambda`表达式在引用对象的声明周期结束后调用，会引发未定义行为。`Lambda`表达式不会延长对象的生命周期。比如通过`this`捕获的当前对象。

### operator ret(*)(params)()

无捕获的非泛型`Lambda`

```CPP
using F = ret(*)(params);
constexpr operator F() const noexcept;
```

无捕获的泛型`Lambda`.

```CPP
template<template-params> using fptr_t = /* see below */;
template<template-params>
constexpr operator fptr_t<template-params>() const noexcept;
```

这个类型转换函数只有在捕获列表为空时才会定义。它是`public`,`constexpr`,`non-virtual`,`non-explicit`,`const`,`noexcept`的函数。

无捕获的泛型`Lambda`定义的转换函数与`operator()`具有相同的模板参数。

```CPP
void f1(int (*)(int)) {}
void f2(char (*)(int)) {}
void h(int (*)(int)) {}  // #1
void h(char (*)(int)) {} // #2
 
auto glambda = [](auto a) { return a; };
f1(glambda); // OK
f2(glambda); // error: not convertible
h(glambda);  // OK: calls #1 since #2 is not convertible
 
int& (*fpi)(int*) = [](auto* a) -> auto& { return *a; }; // OK
```

返回的函数指针调用时，产生的效果就好像调用了一个默认构造的Lambda表达式的`operator()`.

如果函数调用运算符（或泛型 lambda 的特化）为`constexpr`，则此函数为`constexpr`。

```CPP
auto Fwd = [](int(*fp)(int), auto a) { return fp(a); };
auto C = [](auto a) { return a; };
static_assert(Fwd(C, 3) == 3);  // OK
 
auto NC = [](auto a) { static int s; return a; };
static_assert(Fwd(NC, 3) == 3); // error: no specialization can be
                                // constexpr because of static s
```

### ClosureType()

```CPP
ClosureType() = default;
ClosureType(const ClosureType&) = default;
ClosureType(ClosureType&&) = default;
```

默认构造函数只有在无捕获的Lambda表达式下才会定义。

### operator=(const ClosureType&)

```CPP

ClosureType& operator=(const ClosureType&) = default;
ClosureType& operator=(ClosureType&&) = default; (only if no captures are specified)

ClosureType& operator=(const ClosureType&) = delete; (otherwise)
```

只有当没有指定捕获时，才会定义移动赋值函数。

### Captures

```CPP
T1 a;
T2 b;
...
```

如果Lambda表达式以复制方式捕获了任何对象，这个类型就会存储这些对象，标准没有指明顺序。

如果是以引用方式捕获的对象，标准没有指定是否会出现。

## Lambda capture

`lambda`是用于创建匿名类的语法。捕获变量意味着该变量被传递给该类的构造函数。也就是说，变量的捕获发生在声明`Lambda`表达式时，引用捕获的变量生命周期取决于`Lambda`表达式所捕获的变量.

Lambda表达式里的`capture`是一个逗号分隔的列表，可以以默认捕获行为(capture-default)开头，指定了捕获的变量与对应的捕获方式。捕获的变量便可以在Lambda表达式函数体内使用。

默认捕获行为只有两个，如下

* `&`隐式捕获所有具有动态生存周期的变量，捕获方式为引用。
* `=`隐式捕获所有具有动态生存周期的变量，捕获方式为复制。

如果指定了默认的捕获行为，注意，当前的对象`*this`也会被捕获，并且总是以引用方式捕获的。

`capture`的格式如下

* `identifier`以复制捕获指定变量。（简单捕获）
* `identifier ...`以复制捕获指定变量，具有包展开机制。（简单捕获）
* `identifier initializer`以复制捕获指定变量，提供了初始化器。
* `& identifier`以引用捕获指定变量。（简单捕获）
* `& identifier ...`以引用捕获指定变量，具有包展开机制。（简单捕获）
* `& identifier initializer`以引用捕获指定变量，提供了初始化器。
* `this`以引用捕获当前的对象。（简单捕获）
* `* this`以复制捕获当前对象。（简单捕获）
* `... identifier initializer`以复制捕获指定变量，提供了初始化器，具有包展开机制。
* `& ... identifier initializer`以引用捕获指定变量，提供了初始化器，具有包展开机制。

如果默认捕获行为为`&`，后续的任何的简单捕获不能以`&`开头来指定例外。

```CPP
struct S2 { void f(int i); };
void S2::f(int i)
{
    [&] {};          // OK: by-reference capture default
    [&, i] {};       // OK: by-reference capture, except i is captured by copy
    [&, &i] {};      // Error: by-reference capture when by-reference is the default
    [&, this] {};    // OK, equivalent to [&]
    [&, this, i] {}; // OK, equivalent to [&, i]
}
```

如果默认捕获行为为`=`，后续的任何的简单捕获必须以`&`开头来指定例外

```CPP
struct S2 { void f(int i); };
void S2::f(int i)
{
    [=] {};        // OK: by-copy capture default
    [=, &i] {};    // OK: by-copy capture, except i is captured by reference
    [=, *this] {}; // until C++17: Error: invalid syntax
                   // since C++17: OK: captures the enclosing S2 by copy
    [=, this] {};  // until C++20: Error: this when = is the default
                   // since C++20: OK, same as [=]
}
```

每个标识符只能出现一次,并且必须与形参名不同。

```CPP
struct S2 { void f(int i); };
void S2::f(int i)
{
    [i, i] {};        // Error: i repeated
    [this, *this] {}; // Error: "this" repeated (C++17)
 
    [i] (int i) {};   // Error: parameter and capture have the same name
}
```

具有初始化器的捕获就好像是声明并显式捕获了一个使用`auto`定义的变量，并使用相同的初始化器，且声明域是在Lambda表达式的函数体内。

这个可以用于捕获仅移动的类型，比如`x = std::move(x)`.

也可以用于通过常引用捕获，比如`&cr = std::as_const(x)`.

```CPP
int x = 4;
 
auto y = [&r = x, x = x + 1]() -> int
{
    r += 2;
    return x * x;
}(); // updates ::x to 6 and initializes y to 25. 
```

Lambda表达式使用引用来捕获的变量，更像是捕获了指向的变量的地址，

```CPP
#include <iostream>
 
auto make_function(int& x)
{
    return [&] { std::cout << x << '\n'; };
}
 
int main()
{
    int i = 3;
    auto f = make_function(i); // the use of x in f binds directly to i
    i = 5;
    f(); // OK: prints 5
}
```

类成员不能被显式捕获而不添加初始化器

```CPP
class S
{
    int x = 0;
 
    void f()
    {
        int i = 0;
    //  auto l1 = [i, x] { use(i, x); };      // error: x is not a variable
        auto l2 = [i, x = x] { use(i, x); };  // OK, copy capture
        i = 1; x = 1; l2(); // calls use(0,0)
        auto l3 = [i, &x = x] { use(i, x); }; // OK, reference capture
        i = 2; x = 2; l3(); // calls use(1,2)
    }
};
```

当隐式捕获了类成员时，它不会复制类成员，而是通过引用捕获的`this`来使用，类似于`(*this).m`.

```CPP
class S
{
    int x = 0;
 
    void f()
    {
        int i = 0;
 
        auto l1 = [=] { use(i, x); }; // captures a copy of i and
                                      // a copy of the this pointer
        i = 1; x = 1; l1();           // calls use(0, 1), as if
                                      // i by copy and x by reference
 
        auto l2 = [i, this] { use(i, x); }; // same as above, made explicit
        i = 2; x = 2; l2();           // calls use(1, 2), as if
                                      // i by copy and x by reference
 
        auto l3 = [&] { use(i, x); }; // captures i by reference and
                                      // a copy of the this pointer
        i = 3; x = 2; l3();           // calls use(3, 2), as if
                                      // i and x are both by reference
 
        auto l4 = [i, *this] { use(i, x); }; // makes a copy of *this,
                                             // including a copy of x
        i = 4; x = 4; l4();           // calls use(3, 2), as if
                                      // i and x are both by copy
    }
};
```

如果Lambda表达式出现在了函数的默认参数上，它不能捕获任何对象，除非所捕获的对象都具有初始化器。

```CPP
void f2()
{
    int i = 1;
 
    void g1( int = [i] { return i; }() ); // error: captures something
    void g2( int = [i] { return 0; }() ); // error: captures something
    void g3( int = [=] { return i; }() ); // error: captures something
 
    void g4( int = [=] { return 0; }() );       // OK: capture-less
    void g5( int = [] { return sizeof i; }() ); // OK: capture-less
 
    // C++14
    void g6( int = [x = 1] { return x; }() ); // OK: 1 can appear
                                              //     in a default argument
    void g7( int = [x = i] { return x; }() ); // error: i cannot appear
                                              //        in a default argument
}
```

## 例子

本例展示了Lambda表达式用于通用的stl算法里的，以及如何将`lambda`表达式生成的对象存储在`std::function`对象中。

```CPP
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>
 
int main()
{
    std::vector<int> c{1, 2, 3, 4, 5, 6, 7};
    int x = 5;
    c.erase(std::remove_if(c.begin(), c.end(), [x](int n) { return n < x; }), c.end());
 
    std::cout << "c: ";
    std::for_each(c.begin(), c.end(), [](int i) { std::cout << i << ' '; });
    std::cout << '\n';
 
    // the type of a closure cannot be named, but can be inferred with auto
    // since C++14, lambda could own default arguments
    auto func1 = [](int i = 6) { return i + 4; };
    std::cout << "func1: " << func1() << '\n';
 
    // like all callable objects, closures can be captured in std::function
    // (this may incur unnecessary overhead)
    std::function<int(int)> func2 = [](int i) { return i + 4; };
    std::cout << "func2: " << func2(6) << '\n';
 
    constexpr int fib_max {8};
    std::cout << "Emulate `recursive lambda` calls:\nFibonacci numbers: ";
    auto nth_fibonacci = [](int n)
    {
        std::function<int(int, int, int)> fib = [&](int n, int a, int b)
        {
            return n ? fib(n - 1, a + b, a) : b;
        };
        return fib(n, 0, 1);
    };
 
    for (int i{1}; i <= fib_max; ++i)
        std::cout << nth_fibonacci(i) << (i < fib_max ? ", " : "\n");
 
    std::cout << "Alternative approach to lambda recursion:\nFibonacci numbers: ";
    auto nth_fibonacci2 = [](auto self, int n, int a = 0, int b = 1) -> int
    {
        return n ? self(self, n - 1, a + b, a) : b;
    };
 
    for (int i{1}; i <= fib_max; ++i)
        std::cout << nth_fibonacci2(nth_fibonacci2, i) << (i < fib_max ? ", " : "\n");
 
#ifdef __cpp_explicit_this_parameter
    std::cout << "C++23 approach to lambda recursion:\n";
    auto nth_fibonacci3 = [](this auto self, int n, int a = 0, int b = 1) -> int
    {
         return n ? self(n - 1, a + b, a) : b;
    };
 
    for (int i{1}; i <= fib_max; ++i)
        std::cout << nth_fibonacci3(i) << (i < fib_max ? ", " : "\n");
#endif
}
```
