# Using-declaration

参考文档

* CPP reference[Using-declaration](https://en.cppreference.com/w/cpp/language/using_declaration)

将一个在别处定义的名字引入当前的作用域中。

## 语法

```CPP
using declarator-list;
```

## 详细解释

`using`声明可以用于**将命名空间的成员引入其它命名空间或者是块作用域。**也可以用于**将基类成员引入到派生类**里。在`C++ 20`之后，还可以将`enumerators`引入名称空间，块或者类作用域。

使用一个`using`声明多个名字与分别使用`using`声明声明对应的名字没有区别。

在名称空间（块作用域）引入别的名字后，这个名字就好像是在当前名称空间（块作用域）定义的，编译器就会检查是否重复定义。这点和`using`指令不同。 \

### 在名称空间与块作用域中

`using`声明可以将别的名称空间的名字引入**当前的**名称空间**或者**块作用域。

```CPP
#include <iostream>
#include <string>
 
using std::string;
 
int main()
{
    string str = "Example";
    using std::cout;
    cout << str;
}
```

`using std::string`将`std`名称空间的`string`类引入了全局作用域`::`.

`using std::cout`将`std`名称空间的`cout`类引入了`main`函数的块作用域。

### 在类定义中

`using`声明将基类的成员引入到派生类中去，比如我们可以把基类`protect`成员变为派生类的`public`成员。所以，在`using`声明中的作用域列表必须是已经定义的基类。如果`using`声明把一个基类成员函数引入到了派生类，那么**所有该成员函数的重载版本**都会引入到派生类。如果派生类早已有了相同的名字，那么派生类的成员隐藏或者是重载引入的成员。

```CPP
#include <iostream>
 
struct B
{
    virtual void f(int) { std::cout << "B::f\n"; }
    void g(char)        { std::cout << "B::g\n"; }
    void h(int)         { std::cout << "B::h\n"; }
protected:
    int m; // B::m is protected
    typedef int value_type;
};
 
struct D : B
{
    using B::m;          // D::m is public
    using B::value_type; // D::value_type is public
 
    using B::f;
    void f(int) override { std::cout << "D::f\n"; } // D::f(int) overrides B::f(int)
 
    using B::g;
    void g(int) { std::cout << "D::g\n"; } // both g(int) and g(char) are visible
 
    using B::h;
    void h(int) { std::cout << "D::h\n"; } // D::h(int) hides B::h(int)
};
 
int main()
{
    D d;
    B& b = d;
 
//  b.m = 2;  // Error: B::m is protected
    d.m = 1;  // protected B::m is accessible as public D::m
 
    b.f(1);   // calls derived f()
    d.f(1);   // calls derived f()
    std::cout << "----------\n";
 
    d.g(1);   // calls derived g(int)
    d.g('a'); // calls base g(char), exposed via using B::g;
    std::cout << "----------\n";
 
    b.h(1);   // calls base h()
    d.h(1);   // calls derived h()
}
```

输出为

```text
D::f
D::f
----------
D::g
B::g
----------
B::h
D::h
```

### 构造函数继承

当`using`声明声明了直接基类的构造函数，直接基类的所有构造函数都会（忽略成员可见性）都会在初始化派生类时的函数重载决议可见，这个特性叫做构造函数继承

如果重载决议选择了继承的构造函数。这个构造函数的可使用性取决于当使用这个构造函数构造对应的基类时，这个**构造函数在基类的可使用性**。

如果使用这个继承的构造函数构造派生类对象，那么对应的基类也会使用这个构造函数构造该对象中该基类对应的部分，**别的基类与派生类成员使用默认初始化**。

```CPP
struct B1 { B1(int, ...) {} };
struct B2 { B2(double)   {} };
 
int get();
 
struct D1 : B1
{
    using B1::B1; // inherits B1(int, ...)
    int x;
    int y = get();
};
 
void test()
{
    D1 d(2, 3, 4); // OK: B1 is initialized by calling B1(2, 3, 4),
                   // then d.x is default-initialized (no initialization is performed),
                   // then d.y is initialized by calling get()
 
    D1 e;          // Error: D1 has no default constructor
}
 
struct D2 : B2
{
    using B2::B2; // inherits B2(double)
    B1 b;
};
 
D2 f(1.0); // error: B1 has no default constructor
```

```CPP
struct W { W(int); };
 
struct X : virtual W
{
    using W::W; // inherits W(int)
    X() = delete;
};
 
struct Y : X
{
    using X::X;
};
 
struct Z : Y, virtual W
{
    using Y::Y;
};
 
Z z(0); // OK: initialization of Y does not invoke default constructor of X
```

如同成员函数一样，派生类也可以隐藏继承的构造函数。如果继承的构造函数包含了复制或者是移动构造函数，那么编译器也不会自动创建派生类的复制或移动构造函数。

```CPP
struct B1 { B1(int); };
struct B2 { B2(int); };
 
struct D2 : B1, B2
{
    using B1::B1;
    using B2::B2;
 
    D2(int); // OK: D2::D2(int) hides both B1::B1(int) and B2::B2(int)
};
D2 d2(0);    // calls D2::D2(int)
```

当使用在模板类中，`using`声明有关于依赖名字，这个声明被认为是构造函数的情况为，最后的作用域解析符前的名字与解析符后的名字相同。

```CPP
template<class T>
struct A : T
{
    using T::T; // OK, inherits constructors of T
};
 
template<class T, class U>
struct B : T, A<U>
{
    using A<U>::A; // OK, inherits constructors of A<U>
    using T::A;    // does not inherit constructor of T
                   // even though T may be a specialization of A<>
};
```

## 注意点

`using`声明不能声明名称空间，基类析构函数，或者是模板的特例。

```CPP
struct B
{
    template<class T>
    void f();
};
 
struct D : B
{
    using B::f;      // OK: names a template
//  using B::f<int>; // Error: names a template specialization
 
    void g() { f<int>(); }
};
```

`using`声明也不能引入依赖名称的成员模板。

```CPP
template<class X>
struct B
{
    template<class T>
    void f(T);
};
 
template<class Y>
struct D : B<Y>
{
//  using B<Y>::template f; // Error: disambiguator not allowed
    using B<Y>::f;          // compiles, but f is not a template-name
 
    void g()
    {
//      f<int>(0);          // Error: f is not known to be a template name,
                            // so < does not start a template argument list
        f(0);               // OK
    }   
};
```

`using`声明不能在类作用域中引用非基类的名字。

```CPP
class Robot
{
using Eigen::Matrix; //error!
}
```

`using`声明在类中只能用于引用基类的名字，如果想要实现这个功能，请使用`typedef`.