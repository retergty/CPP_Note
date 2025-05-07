# pack

包是一个C++语法实体,包含以下的内容

* 参数包(parameter pack)
  * 模板参数包(template parameter pack)
  * 函数参数包(function parameter pack)
* lambda函数初始化捕获包(lambda init-capture pack)

参考文档

* [Pack](https://en.cppreference.com/w/cpp/language/pack)

模板参数包可以接受零个或者多个模板实参.

函数参数包可以接受零个或者多个函数实参.

lambda函数初始化捕获包可以接受包展开.

## 语法

模板参数包的语法如下.

```CPP
type ... pack-name (optional) (1) 
typename|class ... pack-name (optional) (2) 
type-constraint ... pack-name (optional) (3)
template < parameter-list > typename|class ... pack-name (optional) (4)
```

函数参数包的语法如下

```CPP
pack-name ... pack-param-name (optional) (5) 
```

(1) 具有可选名称的常量模板参数包
(2) 具有可选名称的类型模板参数包
(3) 具有可选名称的约束类型模板参数包
(4) 具有可选名称的模板模板参数包
(5) 具有可选名称的函数参数包

### 例子

可变模板参数类可以使用任意个数的模板实参实例化

```CPP
template<class... Types>
struct Tuple {};
 
Tuple<> t0;           // Types contains no arguments
Tuple<int> t1;        // Types contains one argument: int
Tuple<int, float> t2; // Types contains two arguments: int and float
Tuple<0> t3;          // error: 0 is not a type
```

可变函数模板可以使用任何数量的函数实参调用.

```CPP
template<class... Types>
void f(Types... args);
 
f();       // OK: args contains no arguments
f(1);      // OK: args contains one argument: int
f(2, 1.0); // OK: args contains two arguments: int and double
```

在类的主模板中，模板参数包必须是模板参数列表的最后一个参数.在函数模板中，模板参数包可以出现在列表的较早位置，前提是所有后续参数都可以从函数参数中推导出来，或者具有默认参数.

```CPP
template<typename U, typename... Ts>    // OK: can deduce U
struct valid;
// template<typename... Ts, typename U> // Error: Ts... not at the end
// struct Invalid;
 
template<typename... Ts, typename U, typename=void>
void valid(U, Ts...);    // OK: can deduce U
// void valid(Ts..., U); // Can't be used: Ts... is a non-deduced context in this position
 
valid(1.0, 1, 2, 3);     // OK: deduces U as double, Ts as {int, int, int}
```

## 包展开

```CPP
pattern ... 
```

扩展为零个或多个模式的列表。该模式(pattern)必须至少包含一个包

包名加上省略号就是模式，模式会被展开为零个或者多个的模式实例。模式实例中，包名会按顺序被替换为包的实际元素。对齐说明符的模式实例包元素以空格分隔，其它实例以逗号分隔.

```CPP
template<class... Us>
void f(Us... pargs) {}
 
template<class... Ts>
void g(Ts... args)
{
    f(&args...); // “&args...” is a pack expansion
                 // “&args” is its pattern
}
 
g(1, 0.2, "a"); // Ts... args expand to int E1, double E2, const char* E3
                // &args... expands to &E1, &E2, &E3
                // Us... pargs expand to int* E1, double* E2, const char** E3
```

如果两个包的名称以相同的模式出现，则它们会同时扩展，并且它们必须长度相同.

```CPP
template<typename...>
struct Tuple {};
 
template<typename T1, typename T2>
struct Pair {};
 
template<class... Args1>
struct zip
{
    template<class... Args2>
    struct with
    {
        typedef Tuple<Pair<Args1, Args2>...> type;
        // Pair<Args1, Args2>... is the pack expansion
        // Pair<Args1, Args2> is the pattern
    };
};
 
typedef zip<short, int>::with<unsigned short, unsigned>::type T1;
// Pair<Args1, Args2>... expands to
// Pair<short, unsigned short>, Pair<int, unsigned int> 
// T1 is Tuple<Pair<short, unsigned short>, Pair<int, unsigned>>
 
// typedef zip<short>::with<unsigned short, unsigned>::type T2;
// error: pack expansion contains packs of different lengths
```

对于递归的包展开，内层的包首先展开，之后是外层的包。

```CPP
template<class... Args>
void g(Args... args)
{
    f(const_cast<const Args*>(&args)...); 
    // const_cast<const Args*>(&args) is the pattern, it expands two packs
    // (Args and args) simultaneously
 
    f(h(args...) + args...); // Nested pack expansion:
    // inner pack expansion is "args...", it is expanded first
    // outer pack expansion is h(E1, E2, E3) + args..., it is expanded
    // second (as h(E1, E2, E3) + E1, h(E1, E2, E3) + E2, h(E1, E2, E3) + E3)
}
```

当包中的元素数量为零（空包）时，就会产生空的列表。

```CPP
template<class... Bases> 
struct X : Bases... { };
 
template<class... Args> 
void f(Args... args) 
{
    X<Args...> x(args...);
}
 
template void f<>(); // OK, X<> has no base classes
                     // x is a variable of type X<> that is value-initialized
```

### 函数实参中

当包展开发生在函数实参中时，省略号前最大的表达式就是模式.

```CPP
f(args...);              // expands to f(E1, E2, E3)
f(&args...);             // expands to f(&E1, &E2, &E3)
f(n, ++args...);         // expands to f(n, ++E1, ++E2, ++E3);
f(++args..., n);         // expands to f(++E1, ++E2, ++E3, n);
 
f(const_cast<const Args*>(&args)...);
// f(const_cast<const E1*>(&X1), const_cast<const E2*>(&X2), const_cast<const E3*>(&X3))
 
f(h(args...) + args...); // expands to 
// f(h(E1, E2, E3) + E1, h(E1, E2, E3) + E2, h(E1, E2, E3) + E3)
```

### 括号初始化器中

当包展开发生在括号初始化器，函数风格的cast中时，和在函数实参中的行为一致.

```CPP
Class c1(&args...);             // calls Class::Class(&E1, &E2, &E3)
Class c2 = Class(n, ++args...); // calls Class::Class(n, ++E1, ++E2, ++E3);
 
::new((void *)p) U(std::forward<Args>(args)...) // std::allocator::allocate
```

### 大括号初始化器中

在大括号初始化器中同理.

```CPP
template<typename... Ts>
void func(Ts... args)
{
    const int size = sizeof...(args) + 2;
    int res[size] = {1, args..., 2};
 
    // since initializer lists guarantee sequencing, this can be used to
    // call a function on each element of a pack, in order:
    int dummy[sizeof...(Ts)] = {(std::cout << args, 0)...};
}
```

### 模板函数实参中

包扩展可以在模板参数列表的任何地方使用，只要模板具有与扩展匹配的参数.

```CPP
template<class A, class B, class... C>
void func(A arg1, B arg2, C... arg3)
{
    container<A, B, C...> t1; // expands to container<A, B, E1, E2, E3> 
    container<C..., A, B> t2; // expands to container<E1, E2, E3, A, B> 
    container<A, C..., B> t3; // expands to container<A, E1, E2, E3, B> 
}
```

### 函数参数中

在函数参数列表中，如果参数声明中出现省略号（无论它是否命名函数参数包（如 Args...args）），参数声明就是模式.

```CPP
template<typename... Ts>
void f(Ts...) {}
 
f('a', 1); // Ts... expands to void f(char, int)
f(0.1);    // Ts... expands to void f(double)
 
template<typename... Ts, int... N>
void g(Ts (&...arr)[N]) {}
 
int n[1];
 
g<const char, int>("a", n); // Ts (&...arr)[N] expands to 
                            // const char (&)[2], int(&)[1]
```

也就是说，函数参数中的省略号，可以同时是模式与函数参数包.比如

```CPP
template<class... Args> 
void f(Args... args);
```

就同时定义了模板实参展开的模式与函数参数包.

### 模板参数中

包展开也可以发生在模板参数中.

```CPP
template<typename... T>
struct value_holder
{
    template<T... Values> // expands to a constant template parameter 
    struct apply {};      // list, such as <int, char, int(&)[5]>
};
```

### 基类声明符与成员初始化列表中

```CPP
template<class... Mixins>
class X : public Mixins...
{
public:
    X(const Mixins&... mixins) : Mixins(mixins)... {}
};
```

### sizeof操作符

sizeof操作符也定义了包展开.

```CPP
template<class... Types>
struct count
{
    static const std::size_t value = sizeof...(Types);
};
```

### alignas操作符中

包展开出现在alignas操作符中时，使用空格分隔.

```CPP
template<class... T>
struct Align
{
    alignas(T...) unsigned char buffer[128];
};
 
Align<int, short> a; // the alignment specifiers after expansion are
                     // alignas(int) alignas(short)
                     // (no comma in between)
```

### lambda捕获中

包展开也可以出现在lambda表达式中.

```CPP
template<class... Args>
void f(Args... args)
{
    auto lm = [&, args...] { return g(args...); };
    lm();
}
```
