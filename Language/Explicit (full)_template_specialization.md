# Explicit (full) template specialization

参考文章

* CPP reference[Explicit (full) template specialization](https://en.cppreference.com/w/cpp/language/template_specialization)
* CSDN [C++模板全特化（具体化）与偏特化（部分具体化）详解](https://blog.csdn.net/greywolf0824/article/details/106856397)

模板全特化(explicit specialization)允许我们代替编译器，为不同类型定义不同的模板的方法，这个不是重载，只是一个模板的多个特例。所有的模板都可以进行全特化。

定义模板全特化前，必须先定义主模板(primary template),可以在任何可以定义主模板的作用域处定义模板全特化，比如在类外定义成员函数。

```CPP
namespace N
{
    template<class T> // primary template
    class X { /*...*/ };
    template<>        // specialization in same namespace
    class X<int> { /*...*/ };
 
    template<class T> // primary template
    class Y { /*...*/ };
    template<>        // forward declare specialization for double
    class Y<double>;
}
 
template<> // OK: specialization in same namespace
class N::Y<double> { /*...*/ };
```

## 语法

类模板的全特化语法为

```CPP
template<typename T> class A {/*...*/};
template<> class A<int> A {/*...*/};
```

函数模板的全特化语法为

```CPP
template<typename T> void f(T) {/*...*/};
template<> void f<int>(int) {/*...*/};
template<> void f<>(int) {/*...*/};
template<> void f(int) {/*...*/};
```

如果模板参数可以通过模板参数推断得来，可以不必写`f<>`,`f<int>`.

变量模板的全特化语法为

```CPP
template<typename T> int x = 0;
template<> char y<char> = 'c';
```

函数模板和变量模板的全特化可以允许特化的函数或变量与主模板有不同的`inline`,`const`，`constexpr`等属性，对于某些编译器，还允许变量的全特化为不同的类型。但是，函数模板的全特化必须与主模板**有相同的参数列表**（其中所有模板参数均被实际实参代替）。

```CPP
template<typename T> int y = 0;
template<> const char y<char> = 'c';

template<typename T> void f(T t) { return; };

template<> inline void f<int>(int t) { return; };
```

注意，以下代码不合法

```CPP
template<typename T> void f(T t,int a) { return; };

template<> void f<int>(int t) { return; }; // error！

template<> void f<int>(char t,int a) { return; }; // error！
```

编译器不允许这种代码的原因是，函数参数也是函数签名的一部分，但是模板全特化**不是**函数重载，所以参数必须与主模板一致。

### 在模板类全特化外定义成员

当在模板全特化外定义成员时，如果之前已经定义了这个模板全特化，那么不写`template<>`;如果之前没有定义这个模板全特化，那么必须在成员类，以及成员模板类的成员加上`template<>`.

能在类外定义的有函数，类，静态变量，**非静态变量**是不能在类外定义的。

对于主模板

```CPP
template<typename T>
class Y
{
public:
    struct B
    {
        void f(char);
    };
    int a;

    template<typename U>
    struct C;
    struct D;
    void g(char);
};
```

如果预先定义了全特化，则都不用`template<>`.

```CPP
template<>
class Y<char>
{
    struct B;
    template<typename U>
    struct C;
    static int c;
};

struct Y<char>::B
{
    void f(char);
};

int Y<char>::c = 0;

template<typename U>
struct Y<char>::C
{
    void f(char);
};

void Y<char>::B::f(char)
{
    return;
}

template<typename U>
void Y<char>::C<U>::f(char)
{
    return;
}
```

如果没有预先定义全特化，那么必须在成员类，以及成员模板类的成员加上`template<>`.

```CPP
template<>
struct Y<int>::B
{
    void f(char);
};

template<>
struct Y<int>::D
{
    void g(char);
};

template<>
template<typename U>
struct Y<int>::C
{
    void f(char);
};

void Y<int>::B::f(char)
{
    return;
}

void Y<int>::g(char)
{
    return;
}

```

还有成员模板函数也需要`template<>`

```CPP
template<typename T>
class Z
{
public:
    template<typename X>
    void a(X);

    void b(T);

    template<typename X>
    void c(T, X);
};

template<>
template<typename X>
void Z<int>::a(X) { return; };

template<>
void Z<int>::b(int) { return; };

template<>
template<typename X>
void Z<int>::c(int, X) { return; };
```

也就是只有普通成员函数和静态变量才可以省略`template<>`.

第二种定义全特化的方法有一个好处，就是它会**隐式继承**主模板的函数(当然，如果我们指定了一个函数，那么所有的重载也不会继承，就跟类继承一样)，如果全特化的类只有一两个方法与主模板不同，那么这个会是很好的全特化方法。但是，它只能实例化主模板中**已经声明**的成员，也不能修改`inline`,`const`，`constexpr`等属性,而且可能会由于继承于主模板的部分与全特化的部分相互作用，产生设计问题，慎用！

### 嵌套全特化

对于类内的模板全特化，方法如下啊

```CPP
template<class T1>
struct A
{
    template<class T2>
    struct B
    {
        template<class T3>
        void mf();
    };
};
 
template<>
struct A<int>;
 
template<>
template<>
struct A<char>::B<double>;
 
template<>
template<>
template<>
void A<char>::B<char>::mf<double>();
```

**不能**在外层模板没有全特化时全特化内层模板

## 全特化不是重载

函数特化都**没有引入**一个全新的模板或者模板实例，它们只是对原来的主（或者非特化）模板中已经隐式声明的实例提供**另一种定义**。在概念上，这是一个相对比较重要的现象，也是特化区别于重载模板的关键之处。

**重载决议**时，优先决议出是不是符合常规函数，不存在符合的普通函数，才会再决议出符合的函数主模板，对于**函数模板重载决议**，会**无视**特化存在(标准规定：**重载决议无视模板特化，重载决议发生在主模板之间**)，决议出函数主模板后，如果函数主模板存在符合的具体化函数模板，**才**会调用具体化函数模板；
