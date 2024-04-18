# decltype

参考文件

* CPP reference[decltype specifier](https://en.cppreference.com/w/cpp/language/decltype)
* stackoverflow[What is decltype and how is it used?](https://stackoverflow.com/questions/18815221/what-is-decltype-and-how-is-it-used)

返回指定的表达式或实体的类型。

## 语法

```Plain
decltype ( entity )	(1)
decltype ( expression )	(2)
```

对于`decltype(e)`，返回的类型如下

* 如果`e`是不用括号括起来的`标识符`(比如`a`,`abc`),或者是不用括号括起来的类成员(比如`a.m`)，那么就返回`e`的类型，**不去除**引用和`cv`限定符。
* 否则，如果`e`是`xvalue`,也就是亡值，那么产生`T&&`,`T`就是`e`的类型。
* 否则，如果`e`是`lvalue`，也就是左值，那么产生`T&`,`T`就是`e`的类型。
* 否则，如果`e`是`prvalue`，也就是纯右值，那么产生`T`,`T`就是`e`的类型。

容易混淆的一点是，对于类型

```CPP
int && a = 4;
int c = 0;
decltype((a)) b = c; //b的类型是`int&`
```

因为变量名是左值，所以产生`T&`,`int& &&`通过引用折叠变为`int &`.

## 例子

```CPP
int foo();
int n = 10;

decltype(n) a = 20;             // a is an "int"    [unparenthesized id-expression]

decltype((n)) b = a;            // b is an "int &"  [(n) is an lvalue]
decltype((std::move(n))) c = a; // c is an "int &&" [(std::move(n)) is an xvalue]
decltype(foo()) d = foo();      // d is an "int"    [(foo()) is a prvalue]

decltype(foo()) && r1 = foo();  // int &&
decltype((n)) && r2 = n;        // int & [& && collapses to &]
```

## 接受“两个”参数的`decltype`

以下的例子看似`decltype`接受了两个参数

```CPP
// Non-templated helper struct:
struct _test_has_foo {
    template<class T>
    static auto test(T* p) -> decltype(p->foo(), std::true_type());

    template<class>
    static auto test(...) -> std::false_type;
};

// Templated actual struct:
template<class T>
struct has_foo : decltype(_test_has_foo::test<T>(0))
{};
```

实则不然，这个是逗号表达式，类型取决于最后一个成员的类型，也就是`std::true_type()`.
