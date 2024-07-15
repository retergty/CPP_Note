# 重载决议

函数重载决议的关键就是寻找最匹配的函数，也就是最具体的函数，如果两个函数都不比对方更具体，则重载决议失败。

参考文档

* [Overload resolution](https://en.cppreference.com/w/cpp/language/overload_resolution)
* [洞悉C++函数重载决议](https://zhuanlan.zhihu.com/p/561977606)

重载决议大致分为两个阶段，第一个阶段叫做二级筛选，把所有不合法的函数调用从候选函数集中去除。二级筛选结束后，所有的函数调用都是合法的了，但是会存在匹配等级，也就是第二个阶段的任务，寻找最匹配函数。

`C++20`引入的`requires`约束是在二级筛选处检查，把不满足`requires`的函数去除，之后最匹配函数查找中，会对两个合法的`requires`排等级高低。

## 寻找最匹配函数

最匹配函数(Best viable function)指的是这个函数比其它任何函数都要好，如果有两个函数都不比对方要好，则重载决议失败，报二义性错误。

函数`F1`比`F2`要好，当`F1`的所有参数的隐式转换不比`F2`要坏，且

1. 至少有一个`F1`的参数的隐式转换比`F2`相应的参数隐式转换要好
2. 如果不满足，从`F1`返回类型到正在初始化的类型的标准转换序列优于从`F2`返回类型的标准转换序列
3. 如果不满足，`F1`的返回类型与正在初始化的引用是同一类型的引用（左值或右值），而`F2`的返回类型不是
4. 如果不满足，`F1`是一个非模板函数，但`F2`是模板函数
5. 如果不满足，如果`F1`和`F2`都是模板函数，且`F1`更加具体。
6. 如果不满足，`F1`比`F2`更有`requires`限制条件

## 例子

### 自定义swap函数

```CPP
void swap(const S&,const S&)
using std::swap;
S S1;
S S2;
swap(S1,S2); //call std::swap(S1,S2)
```

由于`std::swap`是模板函数，而`S&`到`const S&`是最佳匹配的隐式转换，有人可能就以为会匹配`swap`，但是，这两个函数的最匹配判断在**第一步**就已经停止。因为`std::swap`不需要进行`const`转换。

```CPP
int f(const int &); // overload #1
int f(int &);       // overload #2 (both references)
 
int g(const int &); // overload #1
int g(int);         // overload #2
 
int i;
int j = f(i); // lvalue i -> int& is better than lvalue int -> const int&
              // calls f(int&)
int k = g(i); // lvalue i -> const int& ranks Exact Match
              // lvalue i -> rvalue int ranks Exact Match
              // ambiguous overload: compilation error
```

### 匹配左值和右值

```CPP
void Fun(const int&);
void Fun(int &&);

int i = 0;
const int ci = 0;
Fun(0);  //call void Fun(int &&);
Fun(i); //call void Fun(const int&);
Fun(ci); //call void Fun(const int&);
Fun(std::move(i)); //call void Fun(int &&);
Fun(std::move(ci)); //call void Fun(const int&);
```

前面四个函数匹配都很显然，但是第五个匹配匹配的是函数`void Fun(const int&);`，因为`std::move(ci)`的类型是`const int &&`,`const int&`可以指向右值，`int &&`不能绑定到`const`引用上。

```CPP
void Fun(const int&);
void Fun(const int &&);

int i = 0;
const int ci = 0;
Fun(0);  //call void Fun(const int &&);
Fun(i); //call void Fun(const int&);
Fun(ci); //call void Fun(const int&);
Fun(std::move(i)); //call void Fun(const int &&);
Fun(std::move(ci)); //call void Fun(const int &&);
```

注意函数`Fun(std::move(i))`的`std::move(i)`的类型是`int &&`，两个函数都要执行`const`转换，转换完毕后，`void Fun(const int &&);`更具体，所以匹配到。

分析上述函数匹配结果，发现`void Fun(int &&);`在`Fun(std::move(ci));`调用时，把右值绑定到了常左值引用上。而`void Fun(const int &&);`则没有。

为了避免函数绑定到右值上，我们可以

```CPP
void Fun(const int&);
void Fun(const int &&) = delete;
```

经典的赋值运算符重载

```CPP
class T
{
  T& operator=(const T&);
  T& operator=(T&&);
}
```

为什么不用`T& operator=(const T&&)`这是因为，右值赋值后，我们可能要修改被移动的对象，使得他安全析构，进一步使用。
