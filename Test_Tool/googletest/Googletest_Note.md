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

### 测试固件(test fixture)

这个是一个对象，可以在多个测试里共享数据。使用测试固件的方法如下

1. 定义一个类，继承`testing::Test`.以`protected`开头。
2. 在这个类里面，声明想要使用的任何对象。
3. 写下默认构造函数或者是`override` `SetUp`函数。用来初始化测试固件类。
4. 写下析构函数或者是`override` `TearDown`函数。用来销毁测试固件类。
5. 使用测试宏`TEST_F()`.

## 断言

`Gtest`的断言是模拟函数调用的宏，如果一个断言失败了，`Gtest`打印失败断言的位置与预先写好的输出信息。

总的来说，断言主要有两大类，第一类形如`ASSERT_*`，当断言失败时退出**当前的函数**，退出当前的函数指的是**调用这个断言的函数**。第一类形如`EXPECT_*`,当断言失败时，打印位置，但是继续运行。

```CPP
ASSERT_EQ(x.size(), y.size()) << "Vectors x and y are of unequal length";

for (int i = 0; i < x.size(); ++i) {
  EXPECT_EQ(x[i], y[i]) << "Vectors x and y differ at index " << i;
}
```

这段代码就是使用了两个断言,`ASSERT_EQ`和`EXPECT_EQ`.

## 测试宏

`Gtest`框架通过宏来驱动测试代码。这种类型的宏就叫做测试宏，简称测试。测试宏就相当于一个函数，我们可以在里面定义断言，断言的结果就是这个测试的结果。

注意！测试的名字**都不能有**下划线`_`.

### TEST

```CPP
TEST(TestSuiteName, TestName) {
  ... statements ...
}
```

定义一个名为`TestName`的独立测试，它在测试套件`TestSuiteName`中,包含一系列的语句`statements`.

比如说，对于下面的函数

```CPP
int Factorial(int n);  // Returns the factorial of n
```

可能的测试如下。

```CPP
// Tests factorial of 0.
TEST(FactorialTest, HandlesZeroInput) {
  EXPECT_EQ(Factorial(0), 1);
}

// Tests factorial of positive numbers.
TEST(FactorialTest, HandlesPositiveInput) {
  EXPECT_EQ(Factorial(1), 1);
  EXPECT_EQ(Factorial(2), 2);
  EXPECT_EQ(Factorial(3), 6);
  EXPECT_EQ(Factorial(8), 40320);
}
```

### TEST_F

```CPP
TEST_F(TestFixtureName, TestName) {
  ... statements ...
}
```

定义一个名为`TestFixtureName`的独立测试，它使用测试固件`TestFixtureName`,此时，测试套件也是`TestFixtureName`

`GoogleTest`会为每个以`TEST_F`定义的测试**在运行时**创建一个**全新的**测试固件，初始化测试固件，运行测试，并在测试结束后销毁测试固件。`GoogleTest`不会在多个测试中复用测试固件。

比如，我们想要测试名为`Queue`类。

```CPP
template <typename E>  // E is the element type.
class Queue {
 public:
  Queue();
  void Enqueue(const E& element);
  E* Dequeue();  // Returns NULL if the queue is empty.
  size_t size() const;
  ...
};
```

首先，我们定义测试固件。

```CPP
class QueueTest : public testing::Test {
 protected:
  void SetUp() override {
     // q0_ remains empty
     q1_.Enqueue(1);
     q2_.Enqueue(2);
     q2_.Enqueue(3);
  }

  // void TearDown() override {}

  Queue<int> q0_;
  Queue<int> q1_;
  Queue<int> q2_;
};
```

由于我们测试固件中没有使用类外资源，默认的析构函数已经足够。

接下来定义测试。

```CPP
TEST_F(QueueTest, IsEmptyInitially) {
  EXPECT_EQ(q0_.size(), 0);
}

TEST_F(QueueTest, DequeueWorks) {
  int* n = q0_.Dequeue();
  EXPECT_EQ(n, nullptr);

  n = q1_.Dequeue();
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*n, 1);
  EXPECT_EQ(q1_.size(), 0);
  delete n;

  n = q2_.Dequeue();
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*n, 2);
  EXPECT_EQ(q2_.size(), 1);
  delete n;
}
```

### TEST_P

```CPP
TEST_P(TestFixtureName, TestName) {
  ... statements ...
}
```

定义一个参数化测试，测试名为`TestName`使用测试固件`TestFixtureName`.此时，测试套件也是`TestFixtureName`。

参数化测试(Value-parameterized tests)允许我们用不同的参数测试代码，而不必写多个测试。

为了写参数化测试，我们应该先定义一个测试固件，这个固件继承`testing::Test`和`testing::WithParamInterface<T>`(后者是一个纯接口)，`T`是参数的类型,可以是任何可复制的类型。也可以直接继承`testing::TestWithParam<T>`,`testing::TestWithParam<T>`继承`testing::Test`与`testing::WithParamInterface<T>`.

```CPP
class FooTest :
    public testing::TestWithParam<absl::string_view> {
  // You can implement all the usual fixture class members here.
  // To access the test parameter, call GetParam() from class
  // TestWithParam<T>.
};

// Or, when you want to add parameters to a pre-existing fixture class:
class BaseTest : public testing::Test {
  ...
};
class BarTest : public BaseTest,
                public testing::WithParamInterface<absl::string_view> {
  ...
};
```

之后，使用`TEST_P`去定义使用这个固件的测试。

```CPP
TEST_P(FooTest, DoesBlah) {
  // Inside a test, access the test parameter with the GetParam() method
  // of the TestWithParam<T> class:
  EXPECT_TRUE(foo.Blah(GetParam()));
  ...
}

TEST_P(FooTest, HasBlahBlah) {
  ...
}

INSTANTIATE_TEST_SUITE_P(MeenyMinyMoe,
                         FooTest,
                         testing::Values("meeny", "miny", "moe"));
```

通过`GetParam()`函数取得当前输入的参数值。

最后，我们使用`INSTANTIATE_TEST_SUITE_P`测试宏指定参数，实例化测试套件。

上述的代码段，`GoogleTest`会运行如下

* `MeenyMinyMoe/FooTest.DoesBlah/0` for `"meeny"`
* `MeenyMinyMoe/FooTest.DoesBlah/1` for `"miny"`
* `MeenyMinyMoe/FooTest.DoesBlah/2` for `"moe"`
* `MeenyMinyMoe/FooTest.HasBlahBlah/0` for `"meeny"`
* `MeenyMinyMoe/FooTest.HasBlahBlah/1` for `"miny"`
* `MeenyMinyMoe/FooTest.HasBlahBlah/2` for `"moe"`

### INSTANTIATE_TEST_SUITE_P

```CPP
INSTANTIATE_TEST_SUITE_P(InstantiationName,TestSuiteName,param_generator)
INSTANTIATE_TEST_SUITE_P(InstantiationName,TestSuiteName,param_generator,name_generator)
```

实例化参数测试套件`TestSuiteName`.

`InstantiationName`表示这个测试套件实例化的名字。这样，我们就可以使用不同的参数实例化同一个测试套件。在测试的输出，`InstantiationName`会加在`TestSuiteName`前面。

`param_generator`是如下的`GoogleTest`提供的函数，用来生成测试参数。定义在`::testing`名称空间。

| param_generator       | Behavior |
| -------------------- | ----------- |
| `Range(begin, end [, step])` | 展开为值 `{begin, begin+step, begin+step+step, ...}`.不包括`end`,`step`默认为1. |
| Values(v1, v2, ..., vN) | 展开为值 `{v1, v2, ..., vN}` |
| `ValuesIn(container)`或 `ValuesIn(begin,end)` | 从C风格的数组或者是`STL`风格的容器，或者是迭代器`[beg,end)`获取值 |
| `Bool()` | 展开为值`{false, true}` |
| `Combine(g1, g2, ..., gN)` | 展开为`std::tuple` |
| `ConvertGenerator<T>(g)` | 将`param_generator`,`static_cast`为`T` |

`name_generator`是函数或者是函数式对象，返回`std::string`给测试加上自定义后缀名。

## 运行测试

测试宏会在`GoogleTest`自动注册,我们不用重新列出所有定义的测试就可以运行它们。我们只需要在`main`函数里的返回语句中使用`RUN_ALL_TESTS()`.

如果一个测试发生了致命错误，那么就会跳过其余的测试。

```CPP
#include "this/package/foo.h"

#include <gtest/gtest.h>

namespace my {
namespace project {
namespace {

// The fixture for testing class Foo.
class FooTest : public testing::Test {
 protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  FooTest() {
     // You can do set-up work for each test here.
  }

  ~FooTest() override {
     // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
  // for Foo.
};

// Tests that the Foo::Bar() method does Abc.
TEST_F(FooTest, MethodBarDoesAbc) {
  const std::string input_filepath = "this/package/testdata/myinputfile.dat";
  const std::string output_filepath = "this/package/testdata/myoutputfile.dat";
  Foo f;
  EXPECT_EQ(f.Bar(input_filepath, output_filepath), 0);
}

// Tests that Foo does Xyz.
TEST_F(FooTest, DoesXyz) {
  // Exercises the Xyz feature of Foo.
}

}  // namespace
}  // namespace project
}  // namespace my

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

`testing::InitGoogleTest()`函数处理命令行与`GoogleTest`有关的参数。
