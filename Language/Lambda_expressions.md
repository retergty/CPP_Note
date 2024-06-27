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

具有显式的模板参数列表

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