# Curiously Recurring Template Pattern

参考文档

* [Curiously Recurring Template Pattern](https://en.cppreference.com/w/cpp/language/crtp)
* wiki [Curiously recurring template pattern](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern#:~:text=The%20curiously%20recurring%20template%20pattern,itself%20as%20a%20template%20argument.)

CRTP(Curiously Recurring Template Pattern)是模板元编程技巧，其基本做法是将派生类作为模板参数传递给它自己的基类。

```CPP
template<class Z>
class Y {};
 
class X : public Y<X> {};
```

## 实现编译期多态

CRTP可以让基类感受到继承类的存在，从而实现编译期多态。

```CPP
#include <cstdio>
 
template <class Derived>
struct Base { void name() { (static_cast<Derived*>(this))->impl(); } };
struct D1 : public Base<D1> { void impl() { std::puts("D1::impl()"); } };
struct D2 : public Base<D2> { void impl() { std::puts("D2::impl()"); } };
 
void test()
{
    // Base<D1> b1; b1.name(); //undefined behavior
    // Base<D2> b2; b2.name(); //undefined behavior
    D1 d1; d1.name();
    D2 d2; d2.name();
}
 
int main()
{
    test();
}
```

这个技巧利用了一个事实，这个事实是成员函数的定义只有再它被调用时才实例化，才会进行依赖名称的查找和匹配，这样，通过`cast`可以绑定到派生类的成员函数上。此外，此时把基`this`指针转换为派生类`this`指针是安全的，因为本来便是派生类在调用继承于基类的方法，`this`本就指向派生类对象。

这个技巧实现了静多态，不会产生运行时的虚函数表查找开销。

## 对象计数器

使用对象计数器的主要目的是获得给定类的对象创建和销毁的统计信息，可以通过`CRTP`简单地实现。

```CPP
template <typename T>
struct counter
{
    static inline int objects_created = 0;
    static inline int objects_alive = 0;

    counter()
    {
        ++objects_created;
        ++objects_alive;
    }
    
    counter(const counter&)
    {
        ++objects_created;
        ++objects_alive;
    }
protected:
    ~counter() // objects should never be removed through pointers of this type
    {
        --objects_alive;
    }
};

class X : counter<X>
{
    // ...
};

class Y : counter<Y>
{
    // ...
};
```

这样，每次`X`被创建，都会独立计数，和`Y`互不影响。

## 多态链

使用`CTRP`可以保持派生类的链。

假设我们有如下的代码

```CPP
class Printer
{
public:
    Printer(ostream& pstream) : m_stream(pstream) {}
 
    template <typename T>
    Printer& print(T&& t) { m_stream << t; return *this; }
 
    template <typename T>
    Printer& println(T&& t) { m_stream << t << endl; return *this; }
private:
    ostream& m_stream;
};
```

我们可以把这些函数连在一起。

```CPP
Printer(myStream).println("hello").println(500);
```

可是，当我们定义了一个派生类时，当我们调用基类的方法时，我们就丢失了派生类的类型信息。

```CPP
class CoutPrinter : public Printer
{
public:
    CoutPrinter() : Printer(cout) {}

    CoutPrinter& SetConsoleColor(Color c)
    {
        // ...
        return *this;
    }
};
```

```CPP
//                           v----- we have a 'Printer' here, not a 'CoutPrinter'
CoutPrinter().print("Hello ").SetConsoleColor(Color.red).println("Printer!"); // compile error
```

这是因为基类方法返回基类的引用，没有对应派生类的方法。

使用`CTRP`可以解决这个问题

```CPP
// Base class
template <typename ConcretePrinter>
class Printer
{
public:
    Printer(ostream& pstream) : m_stream(pstream) {}
 
    template <typename T>
    ConcretePrinter& print(T&& t)
    {
        m_stream << t;
        return static_cast<ConcretePrinter&>(*this);
    }
 
    template <typename T>
    ConcretePrinter& println(T&& t)
    {
        m_stream << t << endl;
        return static_cast<ConcretePrinter&>(*this);
    }
private:
    ostream& m_stream;
};
 
// Derived class
class CoutPrinter : public Printer<CoutPrinter>
{
public:
    CoutPrinter() : Printer(cout) {}
 
    CoutPrinter& SetConsoleColor(Color c)
    {
        // ...
        return *this;
    }
};
 
// usage
CoutPrinter().print("Hello ").SetConsoleColor(Color.red).println("Printer!");
```
