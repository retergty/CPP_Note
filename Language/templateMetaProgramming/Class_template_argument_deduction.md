# Class template argument deduction(CTAD)

参考文档

* CPP reference[Class template argument deduction](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction)

类模版实参推导(Class template argument deduction)发生在模版实参推导(Template argument deduction)和重载决议(overload resolution)前，是一种利用初始化器来推导实参的特性。分为隐式生成的推导指南(Implicitly-generated deduction guides)和显式指定的推导指南(User-defined deduction guides)。

如果未指定模版实参，在以下的条件下，编译器会自动从初始化器推导模版实参。

* 声明新的变量时，使用的初始化器。

```CPP
std::pair p(2, 4.5);     // deduces to std::pair<int, double> p(2, 4.5);
std::tuple t(4, 3, 2.5); // same as auto t = std::make_tuple(4, 3, 2.5);
std::less l;             // same as std::less<void> l;
```

* new表达式

```CPP
template<class T>
struct A
{
    A(T, T);
};
 
auto y = new A{1, 2}; // allocated type is A<int>
```

* 函数风格的类型转换

```CPP
auto lck = std::lock_guard(mtx);     // deduces to std::lock_guard<std::mutex>
std::copy_n(vi1, 3,
    std::back_insert_iterator(vi2)); // deduces to std::back_insert_iterator<T>,
                                     // where T is the type of the container vi2
std::for_each(vi.begin(), vi.end(),
    Foo([&](int i) {...}));          // deduces to Foo<T>,
                                     // where T is the unique lambda type
```

## 隐式生成的推导指南

编译器会根据类的构造函数，生成虚拟的函数，用这个函数进行模版实参推导和重载决议，这个函数的返回类型就是推导的实参类型。

```CPP
template<class T>
struct UniquePtr
{
    UniquePtr(T* t);
};
 
UniquePtr dp{new auto(2.0)};
 
// One declared constructor:
// C1: UniquePtr(T*);
 
// Set of implicitly-generated deduction guides:
 
// F1: template<class T>
//     UniquePtr<T> F(T* p);
 
// F2: template<class T> 
//     UniquePtr<T> F(UniquePtr<T>); // copy deduction candidate
 
// imaginary class to initialize:
// struct X
// {
//     template<class T>
//     X(T* p);         // from F1
//     
//     template<class T>
//     X(UniquePtr<T>); // from F2
// };
 
// direct-initialization of an X object
// with "new double(2.0)" as the initializer
// selects the constructor that corresponds to the guide F1 with T = double
// For F1 with T=double, the return type is UniquePtr<double>
 
// result:
// UniquePtr<double> dp{new auto(2.0)}
```

```CPP
template<class T>
struct S
{
    template<class U>
    struct N
    {
        N(T);
        N(T, U);
 
        template<class V>
        N(V, U);
    };
};
 
S<int>::N x{2.0, 1};
 
// the implicitly-generated deduction guides are (note that T is already known to be int)
 
// F1: template<class U>
//     S<int>::N<U> F(int);
 
// F2: template<class U>
//     S<int>::N<U> F(int, U);
 
// F3: template<class U, class V>
//     S<int>::N<U> F(V, U);
 
// F4: template<class U>
//     S<int>::N<U> F(S<int>::N<U>); (copy deduction candidate)
 
// Overload resolution for direct-list-init with "{2.0, 1}" as the initializer
// chooses F3 with U=int and V=double.
// The return type is S<int>::N<int>
 
// result:
// S<int>::N<int> x{2.0, 1};
```

## 显式指定的推导指南

隐式生成的推导指南有时不好用，比如，当我们需要从迭代器推导出其指向的元素的类型时，我们就要显式指定推导指南。

### 语法

显式指定的推导指南的语法就是类名加上尾置返回类型。

```Text
explicit-specifier(optional) template-name ( parameter-declaration-clause ) -> simple-template-id ;	
```

### 用法

显式指定的推导指南必须命名为类名，并且在类模版的相同作用域内被定义。如果是成员模版类，还要有相同的访问权限。

显式指定的推导指南不是函数，没有函数体。它也不会被名称查找发现，也不会加入重载决议，（除非是和别的推导指南的重载决议）。

```CPP
// declaration of the template
template<class T>
struct container
{
    container(T t) {}
 
    template<class Iter>
    container(Iter beg, Iter end);
};
 
// additional deduction guide
template<class Iter>
container(Iter b, Iter e) -> container<typename std::iterator_traits<Iter>::value_type>;
 
// uses
container c(7); // OK: deduces T=int using an implicitly-generated guide
std::vector<double> v = {/* ... */};
auto d = container(v.begin(), v.end()); // OK: deduces T=double
container e{5, 6}; // Error: there is no std::iterator_traits<int>::value_type
```
