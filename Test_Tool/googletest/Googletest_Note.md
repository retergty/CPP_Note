# GoogleTest

GoogleTest（简称 GTest） 是 Google 开源的一个跨平台的（Liunx、Mac OS X、Windows等）的 C++ 单元测试框架，可以帮助程序员测试 C++ 程序的结果预期。不仅如此，它还提供了丰富的断言、致命和非致命判断、参数化、”死亡测试”等等。

参考文档

* [GoogleTest User’s Guide](https://google.github.io/googletest/)
* 知乎文章[一文掌握谷歌 C++ 单元测试框架 GoogleTest](https://zhuanlan.zhihu.com/p/544491071)
* 知乎文章[技术贴 | 一文掌握 Google Test 框架](https://zhuanlan.zhihu.com/p/661950698)

`GoogleTest`源码地址

* [GoogleTest](https://github.com/google/googletest)

## 基本概念

### 断言(assertions)

断言就是检查条件是否为真的语句，断言的结果可以是成功，非致命错误，致命错误，当致命错误发生时，就会程序退出**当前的函数**。

### 测试(Tests)

测试使用断言去验证代码的行为，如果测试崩溃或者是有错误的断言，测试失败，反之，成功。

### 测试套件(test suite)

测试套件包含一个或者是多个测试，我们把多个测试组合为一个测试套件的操作，可以提高代码的有序度。

### 测试程序(test program)

一个测试程序包含一个或者多个测试套件。

## 断言

`Gtest`的断言是模拟函数调用的宏，如果一个断言失败了，Gtest打印失败断言的位置与预先写好的输出信息。

总的来说，断言主要有两大类，第一类形如`ASSERT_*`，当断言失败时退出**当前的函数**，退出当前的函数指的是**调用这个断言的函数**。第一类形如`EXPECT_*`,当断言失败时，打印位置，但是继续运行。

```CPP
ASSERT_EQ(x.size(), y.size()) << "Vectors x and y are of unequal length";

for (int i = 0; i < x.size(); ++i) {
  EXPECT_EQ(x[i], y[i]) << "Vectors x and y differ at index " << i;
}
```

这段代码就是使用了两个断言,`ASSERT_EQ`和`EXPECT_EQ`.

## 测试宏

`Gtest`框架通过宏来驱动测试代码。这种类型的宏就叫做测试宏，简称测试。

### TEST 宏

该宏定义用来测试其内部代码，其内部断言决定最终的测试结果。

```CPP
TEST(TestSuiteName, TestName) {
  ... statements ...
}
```
