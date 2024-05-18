# static members

参考文档

* [static members](https://en.cppreference.com/w/cpp/language/static)

静态成员(static member)是使用`static`关键字修饰的类成员，静态成员与类实例无关。

## 特点

类静态成员变量与类实例化对象无关，它们是独立的变量，具有静态或者是线程的生命周期。

类静态成员函数与类实例化对象无关，是普通的函数，没有`this`指针。

`static`关键字只用在类成员声明中，不用在静态成员的定义中。

```CPP
class X { static int n; }; // declaration (uses 'static')
int X::n = 1;              // definition (does not use 'static')
```

在类中的声明不是定义，可以声明为不完整类型,除非使用`constexpr`或者是`inline`修饰。

```CPP
struct Foo;
 
struct S
{
    static int a[]; // declaration, incomplete type
    static Foo x;   // declaration, incomplete type
    static S s;     // declaration, incomplete type (inside its own definition)
};
 
int S::a[10]; // definition, complete type
struct Foo {};
Foo S::x;     // definition, complete type
S S::s;       // definition, complete type
```

有两种方法访问类`T`的静态成员`m`，1）`T::m`.2）`E.m`,`E->m`.其中`E`是表达式，表达式类型分别为`T`或`T*`.

```CPP
struct X
{
    static void f(); // declaration
    static int n;    // declaration
};
 
X g() { return X(); } // some function returning X
 
void f()
{
    X::f();  // X::f is a qualified name of static member function
    g().f(); // g().f is member access expression referring to a static member function
}
 
int X::n = 7; // definition
 
void X::f() // definition
{
    n = 1; // X::n is accessible as just n in this scope
}
```

类静态成员也遵守着访问限制规则，(private,public,protected).

## 静态成员函数

静态成员函数不与任何类实例对象相关联，不含有`this`指针。

静态成员函数不能是`virtual`,`const`,`volatile`的，也不能有引用限制符修饰,比如`&&`.

静态成员函数的地址会以普通函数存储，而不是指向成员函数的指针。

## 静态成员变量

静态成员变量不与任何类实例对象相关联，它们总是存在，哪怕目前没有任何类的实例化对象被定义。因此，静态成员变量只有一份，拥有静态或者是线程的生命周期。

静态成员变量不能是`mutable`的。

静态成员变量可以被声明为`inline`的。一个`inline`的成员变量可以在类定义时定义。

```CPP
struct X
{
    inline static int fully_usable = 1; // No out-of-class definition required, ODR-usable
    inline static const std::string class_name{"X"}; // Likewise
 
    static const int non_addressable = 1; // C.f. non-inline constants, usable
                                          // for its value, but not ODR-usable
    // static const std::string class_name{"X"}; // Non-integral declaration of this
                                                 // form is disallowed entirely
};
```

### 常静态成员变量

如果一个整数或者是枚举静态成员变量被定义为`const`，它可以在类内使用常量表达式初始化。注意，只有整数或枚举类可以，其他类需要加上`inline`.

```CPP
struct X
{
    const static int n = 1;
    const static int m{2}; // since C++11
    const static int k;
};
const int X::k = 3;
```

如果静态成员变量是字面值，它可以被定义为`constexpr`，此时它隐式为`inline`的，必须在类内使用常量表达式初始化。

```CPP
struct X
{
    static const int n = 1;
    static constexpr int m = 4;
};
 
const int *p = &X::n, *q = &X::m; // X::n and X::m are ODR-used
const int X::n;             // … so a definition is necessary
constexpr int X::m;         // … (except for X::m in C++17)
```
