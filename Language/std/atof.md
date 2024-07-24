# atof

定义在头文件`<cstdlib>`中，把字符串转换为浮点数`float`.

参考文档

* [std::atof](https://en.cppreference.com/w/cpp/string/byte/atof)

## 函数声明

```CPP
double atof( const char* str );
```

## 描述

把`str`指向的字符串转换为浮点值。

函数丢弃所有前导的空白字符(使用`std::isspace`确定)，直到找到第一个非空白字符。之后它尽可能地接受之后的字符，并把它们转发为浮点数。有效的浮点数可以是如下类型

* 十进制浮点
  * 前面可以有可选的正负号
  * 非空的十进制数字序列，可能包含小数点`.`
  * `e`或`E`后面跟可选的`+`或`-`号表示以`10`为底的指数
* 十六进制浮点
  * 前面有可选的正负号
  * `0x`或`0X`.
  * 非空的十六进制数字序列，可选地包含小数点`.`
  * `p`或`P`后面跟可选的`+`或`-`号表示以`2`为底的指数
* 无穷大表达
  * 前面有可选的正负号
  * `INF`或`INFINITY`忽略大小写
* 非数字表达式
  * 前面有可选的正负号
  * `NAN`或者是`NAN`忽略大小写。

## 例子

```CPP
#include <cstdlib>
#include <iostream>
 
int main()
{
    std::cout << std::atof("0.0000000123") << '\n'
              << std::atof("0.012") << '\n'
              << std::atof("15e16") << '\n'
              << std::atof("-0x1afp-2") << '\n'
              << std::atof("inF") << '\n'
              << std::atof("Nan") << '\n'
              << std::atof("invalid") << '\n';
}
```

输出

```CPP
1.23e-08
0.012
1.5e+17
-107.75
inf
nan
0
```
