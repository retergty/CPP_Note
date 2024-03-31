# Getter Method

本文探讨类用户获得类的私有成员的方法。

常用的三种方法如下。

```CPP
struct MyType { SubType sub; };

class MyClass
{
  private:
     MyType mMyType;
}

MyType MyClass::getMyType() const { return mMyType; }; (1)
const MyType& MyClass::getMyType() const { return mMyType; }; (2)
const MyType* MyClass::getMyType() const { return &mMyType; } (3)
```

注意给这些函数加上`const`修饰符，表示这个函数不会修改类。

`(1)`创建`MyType`类型的副本。它是纯右值(prvalue)，`getMyType().sub`则是亡值(xvalue).

`(2)`创建`MyType`类型的常引用，它是左值(lvalue),`getMyType().sub`也是左值。

不推荐使用`(3)`，不仅有空悬指针的风险，还不符合用户逻辑。

返回私有成员的副本和常引用各有优劣。以下分析在不同用户代码下，这两种代码的行为。

```CPP
MyType my = myclass.getMyType();
```

对于`(1)`，它会创建副本并使用复制构造函数初始化`my`，在`C++17`后，还会进行返回值优化，直接调用复制构造函数初始化`my`.

对于`(2)`,它会直接调用复制构造函数初始化`my`.

```CPP
MyType my;
my = myclass.getMyType();
```

对于`(1)`，它会创建副本，并使用赋值运算符赋值给`my`。

对于`(2)`,它会直接调用赋值运算符赋值给`my`.

```CPP
function(myclass.getMyType())
```

对于`(1)`会匹配接受左值的重载函数

对于`(2)`会匹配接受右值的重载函数

```CPP
myclass.getMyType().method();
```

对于`(1)`，只能使用`const`方法。

对于`(2)`,可以使用任何方法。

## 使用建议

1. 如果复制成员成本过大，通常考虑使用`(2)`.比如，该私有成员是一个容器，复制容器带来的成本过大；或者该成员是一个很大的类；或者该成员使用了动态分配的内存，但是没有定义移动构造和移动赋值函数。
2. 如果不想类用户知道私有成员的更新，通常考虑使用`(1)`.尤其是在多线程开发中，可能会带来竞争访问的问题。
3. 如果想在取得的私有成员处直接修改内容，通常考虑使用`(1)`.比如，取得一个向量后，可能想直接初始化向量。
4. 如果想取得的私有成员可以用在任何方法上，通常考虑使用`(1)`.