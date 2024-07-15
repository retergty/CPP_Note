# 名称查找

参考文档

* [Name lookup](https://en.cppreference.com/w/cpp/language/lookup)

当程序遇到了一个名字，它就把遇到的名字与之前的声明练习起来，这个过程叫做名称查找(name lookup)。

对于函数或者是模板函数名，名称查找可以查找到多个相同名字的声明，还有可能会包含参数依赖查找(argument-dependent lookup).所有查找到的声明进入候选集，如果有模板函数还需进行模板实参推断(Template argument deduction).之后进行函数重载决议(overload resolution).成员访问限制只会在名称查找与重载决议后才考虑。

而对于其它的名字，比如变量，名称空间，类等，名称查找，这种情况下，在当前查找的作用域下，名称查找只能查找到一个声明，否则会报二义性错误(ambiguous).

名称查找主要分为两大类，

1. 有修饰名称查找(Qualified Name Lookup)。
2. 无修饰名称查找(Unqualified Name Lookup)。

## 有修饰名称查找

参考文档

* [Qualified name lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup)

有修饰名称指的就是出现在作用域修饰符`::`右边的名字。一个修饰名称可以是

* 类成员,包括类成员函数(`static`或`non-static`),类型，模板等
* 名称空间成员
* 枚举

有一个特殊的名称空间`::`叫做全局名称空间，如果`::`左侧没有任何内容，则查找仅考虑在全局命名空间范围中进行的声明,包括使用`using declaration`引入的名称。

```CPP
#include <iostream>
 
int main()
{
    struct std {};
 
    std::cout << "fail\n"; // Error: unqualified lookup for 'std' finds the struct
    ::std::cout << "ok\n"; // OK: ::std finds the namespace std
}
```

在对`::`右侧的名称执行名称查找之前，必须完成对其左侧的名称的查找（除非使用 decltype 表达式，或者左侧没有任何内容）。左侧的查找，可能是有修饰名称的，也有可能是无修饰的，取决于是否另一个`::`在它前面。但是，它只考虑名称空间，类类型，枚举，或者特化为类型的模板。如果不是，程序错误。

```CPP
struct A
{
    static int n;
};
 
int main()
{
    int A;
    A::n = 42; // OK: unqualified lookup of A to the left of :: ignores the variable
    A b;       // Error: unqualified lookup of A finds the variable A
}
 
template<int>
struct B : A {};
 
namespace N
{
    template<int>
    void B();
 
    int f()
    {
        return B<0>::n; // Error: N::B<0> is not a type
    }
}
```

当一个修饰名称被用作声明器，之后的非修饰名称的查找会在修饰名称的类或名称空间开始查找。

```CPP
class X {};
 
constexpr int number = 100;
 
struct C
{
    class X {};
    static const int number = 50;
    static X arr[number];
};
 
X C::arr[number], brr[number];    // Error: look up for X finds ::X, not C::X
C::X C::arr[number], brr[number]; // OK: size of arr is 50, size of brr is 100
```

### 类成员查找

如果`::`左边是类或者联合体名，那么右边的名字将在该类的范围内查找，因此可能会找到该类或其基类的成员的声明。

有修饰名称查找可用于访问由嵌套声明或派生类隐藏的类成员。对有修饰名称查找的成员函数永远都不是`virtual`的。

```CPP
struct B { virtual void foo(); };
 
struct D : B { void foo() override; };
 
int main()
{
    D x;
    B& b = x;
 
    b.foo();    // Calls D::foo (virtual dispatch)
    b.B::foo(); // Calls B::foo (static dispatch)
}
```

还有一种方法是通过`.`与`->`操作符，也会触发类成员查找。

```CPP
struct S {
    void f() {}
};

S s;
s.f();
S* ps = &s;
ps->f();
```

### 名称空间查找

如果`::`左边是名称空间，或者没有什么名称（全局名称空间），则右边的名字在该名称空间内查找。除非是模板实参。

```CPP
namespace N
{
    template<typename T>
    struct foo {};
 
    struct X {};
}
 
N::foo<X> x; // Error: X is looked up as ::X, not as N::X
```

在名称空间`N`的修饰名称查找会**首先**考虑所有当前在`N`内的声明，以及其的`inline`名称空间。如果没有，**之后**会考虑所有使用`using namespace`的名称空间，如果没有，再递归进行。

```CPP
int x;
 
namespace Y
{
    void f(float);
    void h(int);
}
 
namespace Z
{
    void h(double);
}
 
namespace A
{
    using namespace Y;
    void f(int);
    void g(int);
    int i;
}
 
namespace B
{
    using namespace Z;
    void f(char);
    int i;
}
 
namespace AB
{
    using namespace A;
    using namespace B;
    void g();
}
 
void h()
{
    AB::g();  // AB is searched, AB::g found by lookup and is chosen AB::g(void)
              // (A and B are not searched)
 
    AB::f(1); // First, AB is searched. There is no f
              // Then, A, B are searched
              // A::f, B::f found by lookup
              // (but Y is not searched so Y::f is not considered)
              // Overload resolution picks A::f(int)
 
    AB::x++;  // First, AB is searched. There is no x
              // Then A, B are searched. There is no x
              // Then Y and Z are searched. There is still no x: this is an error
 
    AB::i++;  // AB is searched. There is no i
              // Then A, B are searched. A::i and B::i found by lookup: this is an error
 
    AB::h(16.8); // First, AB is searched. There is no h
                 // Then A, B are searched. There is no h
                 // Then Y and Z are searched
                 // Lookup finds Y::h and Z::h. Overload resolution picks Z::h(double)
}
```

允许多次找到相同的声明

```CPP
namespace A { int a; }
 
namespace B { using namespace A; }
 
namespace D { using A::a; }
 
namespace BD
{
    using namespace B;
    using namespace D;
}
 
void g()
{
    BD::a++; // OK: finds the same A::a through B and through D
}
```

## 无修饰名称查找

参考文档

* [Unqualified name lookup](https://en.cppreference.com/w/cpp/language/unqualified_lookup)

无修饰名称和有修饰名称相反，就是没有出现在`::`右边的名字，它依次地查找特定地作用域，直到找到声明或者报错。注意，有些上下文的名称查找会跳过一些声明，比如用在`::`左边的名字会忽略函数，变量与枚举声明等。

为了无修饰名称查找，使用`using directive`引入的`using namespace`的名字好像是声明在一个最近的包括`using directive`使用的名称空间与要引入的名称空间的名称空间中。

此外，对函数的无修饰名称查找还会使用参数依赖查找(argument-dependent lookup)ADL.

### 文件作用域

对于用在全局命名空间的名字，查找使用这个名字**前**的全局作用域。

```CPP
int n = 1;     // declaration of n
int x = n + 1; // OK: lookup finds ::n
 
int z = y - 1; // Error: lookup fails
int y = 2;     // declaration of y
```

### 名称空间作用域

对于用在名称空间，且不在任何函数，类之中的名字，查找使用这个名字**前**的这部分的名称空间，之后是**包括**这个名称空间的名称空间**前**的部分，直到查找完毕全局名称空间。

```CPP
int n = 1; // declaration
 
namespace N
{
    int m = 2;
 
    namespace Y
    {
        int x = n; // OK, lookup finds ::n
        int y = m; // OK, lookup finds ::N::m
        int z = k; // Error: lookup fails
    }
 
    int k = 3;
}
```

### 在名称空间之外定义

对于使用在名称空间外定义语句的名字，查找以它定义在名称空间之内进行。

```CPP
namespace X
{
    extern int x; // declaration, not definition
    int n = 1;    // found 1st
}
 
int n = 2;        // found 2nd
int X::x = n;     // finds X::n, sets X::x to 1
```

### 非成员函数定义

对于使用在函数定义里的名字，包括函数体与默认参数，且这个函数是用户定义或者全局名称空间的成员，会从**内层作用域**开始直到遇到函数体，**之后**是函数声明所在的名称空间（会搜索直到函数定义那一行），**之后**是外层名称空间，**直到**全局名称空间为止。

```CPP
namespace A
{
    namespace N
    {
        void f();
        int i = 3; // found 3rd (if 2nd is not present)
    }
 
    int i = 4;     // found 4th (if 3rd is not present)
}
 
int i = 5;         // found 5th (if 4th is not present)
 
void A::N::f()
{
    int i = 2;     // found 2nd (if 1st is not present)
 
    while (true)
    {
       int i = 1;  // found 1st: lookup is done
       std::cout << i;
    }
}
 
// int i;          // not found
 
namespace A
{
    namespace N
    {
        // int i;  // not found
    }
}
```

### 类定义

对于使用在类定义里的名字，包括基类名字与嵌套类定义。但不包括成员函数体，它的默认参数，异常限定符与默认成员初始化器，会按照以下的作用域搜索

1. 类定义直到使用这个名字处。
2. 基类的所有类定义体，包括基类的基类，递归进行。
3. 如果这个类是嵌套类，外部的类定义体直到这个类的定义处，包括外部类定义的基类。
4. 如果类是`local`的（也就是出现在函数内的类定义），或嵌套在`local`类里，查找类定义的块作用域。
5. 如果类是名称空间的成员，或者嵌套在名称空间成员类里，或者是名称空间成员函数的`local`类，名称空间的作用域直到类的定义处。并继续查找外围名称空间直到全局名称空间。

对于友元声明，确定它是否引用先前声明的实体的查找按上述方式进行，只是它在最内部的封闭命名空间之后停止。

```CPP
namespace M
{
    // const int i = 1; // never found
 
    class B
    {
        // static const int i = 3;     // found 3rd (but will not pass access check)
    };
}
 
// const int i = 5;                    // found 5th
 
namespace N
{
    // const int i = 4;                // found 4th
 
    class Y : public M::B
    {
        // static const int i = 2;     // found 2nd
 
        class X
        {
            // static const int i = 1; // found 1st
            int a[i]; // use of i
            // static const int i = 1; // never found
        };
 
        // static const int i = 2;     // never found
    };
 
    // const int i = 4;                // never found
}
 
// const int i = 5;                    // never found
```

### 注入类名

对于在类或模板类定义中使用该类的名字，无修饰名称查找发现它好像是这个名字是类成员声明。

### 成员函数定义

对于使用在成员函数体，成员函数默认参数，异常限定符与默认成员初始化器的名字，搜索的作用域顺序如同类定义顺序一般，只不过会搜索全部的类作用域，而不只是名称前的类作用域。对于嵌套的类也是一样的。就好像成员函数定义在类末尾一样。而且类外定义的成员函数，搜索的名称空间范围也有所增加。

```CPP
class B
{
    // int i;         // found 3rd
};
 
namespace M
{
    // int i;         // found 5th
 
    namespace N
    {
        // int i;     // found 4th
 
        class X : public B
        {
            // int i; // found 2nd
            void f();
            // int i; // found 2nd as well
        };
 
        // int i;     // found 4th
    }
}
 
// int i;             // found 6th
 
void M::N::X::f()
{
    // int i;         // found 1st
    i = 16;
    // int i;         // never found
}
 
namespace M
{
    namespace N
    {
        // int i;     // never found
    }
}
```

### 友元函数定义

对于使用在友元函数定义的名字，如果友元定义直接定义在类内，那么按照类成员函数顺序查找；如果友元定义在类外，那么按照名称空间里的函数顺序查找。

```CPP
int i = 3;                     // found 3rd for f1, found 2nd for f2
 
struct X
{
    static const int i = 2;    // found 2nd for f1, never found for f2
 
    friend void f1(int x)
    {
        // int i;              // found 1st
        i = x;                 // finds and modifies X::i
    }
 
    friend int f2();
 
    // static const int i = 2; // found 2nd for f1 anywhere in class scope
};
 
void f2(int x)
{
    // int i;                  // found 1st
    i = x;                     // finds and modifies ::i
}
```

### 友元函数声明

对于使用在友元函数声明的名字，且友元函数是另一个类的成员函数，且名字不是任何模板实参的一部分，首先查找成员函数的类作用域。如果没有该名字（或者该名字是模板实参的一部分），查找如同类成员函数顺序。

```CPP
template<class T>
struct S;
 
// the class whose member functions are friended
struct A
{ 
    typedef int AT;
 
    void f1(AT);
    void f2(float);
 
    template<class T>
    void f3();
 
    void f4(S<AT>);
};
 
// the class that is granting friendship for f1, f2 and f3
struct B
{
    typedef char AT;
    typedef float BT;
 
    friend void A::f1(AT);    // lookup for AT finds A::AT (AT found in A)
    friend void A::f2(BT);    // lookup for BT finds B::BT (BT not found in A)
    friend void A::f3<AT>();  // lookup for AT finds B::AT (no lookup in A, because
                              //     AT is in the declarator identifier A::f3<AT>)
};
 
// the class template that is granting friendship for f4
template<class AT>
struct C
{
    friend void A::f4(S<AT>); // lookup for AT finds A::AT
                              // (AT is not in the declarator identifier A::f4)
};
```

## 参数依赖查找

参考文档

* [Argument-dependent lookup (ADL)](https://en.cppreference.com/w/cpp/language/adl)

参数依赖查找`Argument-dependent lookup (ADL)`是用于查找无修饰名称的函数声明时使用的方法，参数依赖查找给普通的无修饰名称查找增加了一条查找路径，查找实参所在的名称空间。

这个查找方法是的运算符重载更加简便

```CPP
#include <iostream>
 
int main()
{
    std::cout << "Test\n"; // There is no operator<< in global namespace, but ADL
                           // examines std namespace because the left argument is in
                           // std and finds std::operator<<(std::ostream&, const char*)
    operator<<(std::cout, "Test\n"); // Same, using function call notation
 
    // However,
    std::cout << endl; // Error: “endl” is not declared in this namespace.
                       // This is not a function call to endl(), so ADL does not apply
 
    endl(std::cout); // OK: this is a function call: ADL examines std namespace
                     // because the argument of endl is in std, and finds std::endl
 
    (endl)(std::cout); // Error: “endl” is not declared in this namespace.
                       // The sub-expression (endl) is not an unqualified-id
}
```

首先，如果普通的无修饰名称查找找到的查找集包含以下之一，ADL不会发生

* 类成员的声明
* 块作用域内的函数声明
* 不是函数或者是函数模板的声明

否则，对于函数调用的每一个实参，会检查其类型，确定将要添加到查找中的命名空间和类。这些添加是递归的，比如添加了类型之后，又会考虑这个类型匹配的条，直到所有递归结束。

* 如果实参是基本类型，没有对于的命名空间与类
* 如果实参是类，把以下加入
  * 类本身，
  * 所有它的基类（递归地）
  * 如果该类是某个类的成员，这个类
  * 加入的所有类（包括基类）所在的最内层的名称空间。
* 如果实参是模板类特例化，除了上述的，还会添加
  * 所有模板实参的类型
  * 模板模板参数类型的命名空间
  * 模板模板实参所在的类
* 如果实参是枚举类，枚举类声明的最内层名称空间，如果枚举类是类成员，则把该类添加。
* 如果实参是指向`T`的指针，或指向`T`数组的指针，`T`也会检查
* 如果实参是函数类型，函数参数类型与函数返回值类型也会考虑

确定完所有的命名空间和类并查找完毕后，**所有在类内发现的声明会直接去除**，也就是说，类只不过是为了查找名称空间而引入的中间件罢了。

之后，所有的普通无修饰名称查找与参数依赖查找结合起来，就是所有找到的函数声明集合，之后进入函数重载决议。

使用参数依赖查找可以实现ADL二段式

```CPP
namespace mylib {

    struct S {};

    void swap(S&, S&) {}

    void play() {
        using std::swap; 

        S s1, s2;
        swap(s1, s2); // OK, found by Unqualified Name Lookup

        int a1, a2;
        swap(a1, a2); // OK, found by using declaration
    }
}
```

这样，如果对于`S`就会使用自定义的`swap`函数，对于其它的函数就会使用`std::swap`。

但如果

```CPP
namespace mylib {

    struct S {};

    void swap(S&, S&) {} // #1

    void play() {
        using namespace std;

        S s1, s2;
        swap(s1, s2); // OK, found by Unqualified Name Lookup

        int a1, a2;
        swap(a1, a2); // Error
    }
}
```

就会出错，因为`using namespace std;`把`std::swap`引入了包含两者最近的名称空间，也就是全局名称空间，此时普通的无修饰查找查找到`void swap(S&, S&) {}`就停下来了。不会继续查找。

### 例子

```CPP
namespace A {
    // S2
    struct Base {};
}

namespace M {
    // S3 not works!
    namespace B {
        // S1
        struct Derived : A::Base {};
    }
}

int main() {
    M::B::Derived d;
    f(d); // #1
}
```

```CPP
namespace C {
    struct Final {};
    void g(...) {
        std::cout << "g found by ADL\n";
    }
};

namespace B {
    template <typename T>
    struct Temtem {};

    struct Bar {};
    void f(...) {
        std::cout << "f found by ADL\n";
    }
}

namespace A {
    template <typename T>
    struct Foo {};
}

int main() {
    // class template arguments
    A::Foo<B::Bar> foo;
    f(foo); // OK

    // template template arguments
    A::Foo<B::Temtem<C::Final>> a;
    g(a); // OK

}
```

## 依赖名称

参考文档

* [Dependent names](https://en.cppreference.com/w/cpp/language/dependent_name#Lookup_rules)

依赖名称(dependent name)指的是在模板定义时依赖于模板参数的名称，这些名称不能在模板定义是可知，所以对其的查找延后到模板实例化阶段。

常见的情况是，基类依赖于模板参数，进行无修饰名称查找不会查找到其内部的成员，需要变为依赖名称并进行有修饰名称查找。

```CPP
template <typename T>
struct Base {
    void f() {
        std::cout << "Base class\n";
    }
};

template <typename T>
struct Derived : Base<T> {
    void h() {
        std::cout << "Derived class\n";
        f(); // error: use of undeclared identifier 'f'
        this->f(); // right
    }
};


int main() {
    Derived<int> d;
    d.h();
}
```

## 模板定义的情况

由于模板定义时还不知道传入的模板实参，所以依赖名称(dependent name)的查找会延后到模板实参确定之后,且

* `non-ADL`只会在模板定义处的**上下文**查找，按照无修饰名称或者是有修饰名称对应的规则查找，查找的范围可能扩大。
* `ADL`会在模板定义与模板实例化的上下文查找。

```CPP
// an external library
namespace E
{
    template<typename T>
    void writeObject(const T& t)
    {
        std::cout << "Value = " << t << '\n';
    }
}
 
// translation unit 1:
// Programmer 1 wants to allow E::writeObject to work with vector<int>
namespace P1
{
    std::ostream& operator<<(std::ostream& os, const std::vector<int>& v)
    {
        for (int n : v)
            os << n << ' ';
        return os;
    }
 
    void doSomething()
    {
        std::vector<int> v;
        E::writeObject(v); // error: will not find P1::operator<<
    }
}
```

由于`std::cout << "Value = " << t << '\n';`进行`non-ADL`查找时，只会在模板定义上下文处查找，查找找使用这个名字**前**的这部分的名称空间，这个"前“指的模板实例化前。所以

```CPP
// an external library
namespace E
{
    template<typename T>
    void writeObject(const T& t)
    {
        std::cout << "Value = " << t << '\n';
    }
    std::ostream& operator<<(std::ostream& os, const std::vector<int>& v)
    {
        for (int n : v)
            os << n << ' ';
        return os;
    }
 
}
 
// translation unit 1:
// Programmer 1 wants to allow E::writeObject to work with vector<int>
namespace P1
{
    void doSomething()
    {
        std::vector<int> v;
        E::writeObject(v); // error: will not find P1::operator<<
    }
}
```

也是可以编译成功的。

或者

```CPP
namespace P1
{
    // if C is a class defined in the P1 namespace
    std::ostream& operator<<(std::ostream& os, const std::vector<C>& v)
    {
        for (C n : v)
            os << n;
        return os;
    }
 
    void doSomething()
    {
        std::vector<C> v;
        E::writeObject(v); // OK: instantiates writeObject(std::vector<P1::C>)
                           //     which finds P1::operator<< via ADL
    }
}
```
