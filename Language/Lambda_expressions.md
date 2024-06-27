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

* `operator()(params)`

  ```CPP
  ret operator()(params) { body }
  template<template-params>
  ret operator()(params) { body }
  ```

  处理的函数体。当访问变量时，取决于捕获的类型决定是访问变量的副本(copy)还是访问变量的引用(reference)。

  `parms`就是Lambda表达式中声明的`parms`,如果没有则为空。

  `ret`的类型就是Lambda表达式中声明的`trailing-type`,如果没有声明，则自动推断返回值类型。