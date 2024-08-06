# function

定义在`<functional>`头文件，是通用的函数包装器。

参考文档

* CPP Reference[function](https://en.cppreference.com/w/cpp/utility/functional/function)

## 类声明

```CPP
template< class >
class function; /* undefined */
template< class R, class... Args >
class function<R(Args...)>;
```

* `R(Args...)`是函数返回值与参数类型。

## 描述

`std::function`是通用的函数包装器，可以存储所有可调用对象，包括指向函数的指针，`Lambda`表达式，`bind`表达式，定义了`operator()`的类，以及指向成员函数以及成员变量的指针。

所存储的可调用对象叫做`std::function`的目标(`target`),如果`std::function`不包含目标，它就是**空的**。在一个空的`std::function`上调用目标会抛出`std::bad_function_call`异常。

`std::function`满足复制可构造以及复制可赋值。

## 成员类型

* `result_type`返回类型`R`.

## 成员函数

### 构造与析构

* [function](https://en.cppreference.com/w/cpp/utility/functional/function/function)

  ```CPP
  function() noexcept;

  function( std::nullptr_t ) noexcept;

  function( const function& other );

  function( function&& other ) noexcept;

  template< class F >
  function( F&& f );
  ```

  构造`std::function`.

  `5`使用`std::forward<F>(f)`初始化目标,如果`f`是指向函数的`NULL`指针或者`std::function`的空值，`*this`为空。这个构造函数可以用于类型转换的`std::function`.比如`std::function<bool(int)>`可以用来初始化`std::function<void(int)>`.如果返回值存在类型转换(包括用户自定义的类型转换)或者是`void`类型，那么便可以初始化。通用的原则是，新产生的`std::function`不能扩大原`std::function`的使用范围。

* [operator=](https://en.cppreference.com/w/cpp/utility/functional/function/operator%3D)

  ```CPP
  function& operator=( const function& other );

  function& operator=( function&& other );

  function& operator=( std::nullptr_t ) noexcept;

  template< class F >
  function& operator=( F&& f );

  template< class F >
  function& operator=( std::reference_wrapper<F> f ) noexcept;
  ```

  赋值函数

## 例子

```CPP
#include <functional>
#include <iostream>
 
struct Foo
{
    Foo(int num) : num_(num) {}
    void print_add(int i) const { std::cout << num_ + i << '\n'; }
    int num_;
};
 
void print_num(int i)
{
    std::cout << i << '\n';
}
 
struct PrintNum
{
    void operator()(int i) const
    {
        std::cout << i << '\n';
    }
};
 
int main()
{
    // store a free function
    std::function<void(int)> f_display = print_num;
    f_display(-9);
 
    // store a lambda
    std::function<void()> f_display_42 = []() { print_num(42); };
    f_display_42();
 
    // store the result of a call to std::bind
    std::function<void()> f_display_31337 = std::bind(print_num, 31337);
    f_display_31337();
 
    // store a call to a member function
    std::function<void(const Foo&, int)> f_add_display = &Foo::print_add;
    const Foo foo(314159);
    f_add_display(foo, 1);
    f_add_display(314159, 1);
 
    // store a call to a data member accessor
    std::function<int(Foo const&)> f_num = &Foo::num_;
    std::cout << "num_: " << f_num(foo) << '\n';
 
    // store a call to a member function and object
    using std::placeholders::_1;
    std::function<void(int)> f_add_display2 = std::bind(&Foo::print_add, foo, _1);
    f_add_display2(2);
 
    // store a call to a member function and object ptr
    std::function<void(int)> f_add_display3 = std::bind(&Foo::print_add, &foo, _1);
    f_add_display3(3);
 
    // store a call to a function object
    std::function<void(int)> f_display_obj = PrintNum();
    f_display_obj(18);
 
    auto factorial = [](int n)
    {
        // store a lambda object to emulate "recursive lambda"; aware of extra overhead
        std::function<int(int)> fac = [&](int n) { return (n < 2) ? 1 : n * fac(n - 1); };
        // note that "auto fac = [&](int n) {...};" does not work in recursive calls
        return fac(n);
    };
    for (int i{5}; i != 8; ++i)
        std::cout << i << "! = " << factorial(i) << ";  ";
    std::cout << '\n';
}
```
