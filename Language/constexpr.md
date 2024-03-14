# constexpr

参考文档

* cpp reference[constexpr specifier](https://en.cppreference.com/w/cpp/language/constexpr)
* CSDN[C++之constexpr详解](https://blog.csdn.net/janeqi1987/article/details/103542802)
* [What are 'constexpr' useful for](https://stackoverflow.com/questions/27473795/what-are-constexpr-useful-for)
* [What's the difference between constexpr and const](https://stackoverflow.com/questions/14116003/whats-the-difference-between-constexpr-and-const)

`constexpr`修饰符表明可以在编译期获取到它修饰的函数或者是变量的值，这些变量或函数就可以用在需求常量表达式(constant expressions)的地方，加快了运行速度。

声明`constexpr`变量默认这个变量为`const`.声明`constexpr`函数或者是静态成员变量默认为`inline`

## constexpr变量

定义`constexpr`变量有如下的要求

* `constexpr`变量类型必须为字面类型，通常包括内置类型和定义了`constexpr`构造函数的类型。
* `constexpr`变量必须立即初始化
* `constexpr`变量初始化的整个表达式，包括所有的隐式类型转换、构造函数等，必须是常量表达式*constant expressions*。

内置类型的`constexpr`如下

```CPP
constexpr float x = 42.0;
constexpr float y{108};
constexpr float z = exp(5, 3);
constexpr int i; // Error! Not initialized
int j = 0;
constexpr int k = j + 1; //Error! j not a constant expression
```

用户自定义的`constexpr`如下

```CPP
class S
{
public:
    constexpr S(int val) : _val(val) {};
    int _val;
};
```

constexpr变量一定是常量表达式，如果编译器不能初始化这个constexpr变量，那么就会报错。

## constexpr函数

`constexpr`函数有如下要求

* 不应该是`virtual`的
* 不应该包含异常处理
* 对于`constexpr`构造函数，类不应该有虚基类。
* 返回值与它的参数必须是字面类型
* 至少存在一组实参使得函数调用是核心常量表达式
* 函数体不能包含`go to`,定义非字面类型的变量，定义具有静态或者线程生命周期的变量
* 函数可以修改声明周期和常量表达式相同的对象。

`C++14`前，要求`constexpr`函数只能包含一行`return`语句，但是之后放宽了。

`constexpr`函数**不一定是**常量表达式，取决于调用的实参是否是编译期可知，如果不可知，函数退化为运行期函数。

### constexpr构造函数

`constexpr`构造函数还添加了额外的要求

* 对于类的`constexpr`构造函数，每个成员都应该初始化。
* 对于`union`的`constexpr`构造函数，需要初始化其中一个成员。
* 同时，用来初始化基类和成员的构造函数也应该是`constexpr`的。

### constexpr析构函数

在C++20以前，不允许定义`constexpr`析构函数，但是在`C++20`后放宽了。

### constexpr模板函数

对于`constexpr`模板函数和`constexpr`模板类的成员函数，至少一个特化要满足以上对于函数的要求。此时，其他的特化也会被认为是`constexpr`，但是不满足上述要求的特化不能出现在常量表达式上下文里。

## 注意点

`constexpr`变量保证这个变量是可以在编译期计算出来的。`constexpr`函数返回值可以不是常量表达式，取决于调用的实参，如果实参**可以在编译期计算而来**，那么返回值就是常量表达式，否则不是。这个特性带来了使用上的问题。

* 在`constepxr`函数内使用`constexpr`变量，注意，`constexpr`函数的参数不是常量表达式。

    ```CPP
    constexpr int t1(const int i)
    {
        constexpr int ii = i;  // error! occurs here (i is not a constant expression)
        return ii;
    }
    ```

    由于`constexpr`函数会退化的特性，所以`constexpr`函数的形参不是常量表达式，它能不能在编译期取得它的值取决于我们传递给它的实参，比如

    ```CPP
    int i = 0;
    t1(i); // 实参不能在编译期得出

    const int ci = 0;
    t1(ci); // 实参可以在编译期得出
    ```

    注意，似乎`i`可以在编译期得出，但是可能在另一个线程改变`i`，编译期不能做这么危险的假设。

    此外，`constexpr`变量必须立即被常量表达式初始化，当编译器运行到这一行时，无法马上初始化这个`constexpr`变量。如果我们只是想要这个函数在编译期运行，这样写也是可以的

    ```CPP
    constexpr int t1(const int i)
    {
        int ii = i;  // error! occurs here (i is not a constant expression)
        return ii;
    }
    constexpr int a = t1(0);
    ```

* `const`变量与常量表达式不同，不是任何时候都可以转化为常量表达式

    ```CPP
    int b = 0;
    const int a = b;
    constexpr int cspa = a; // error! expression did not evaluate to a constant. failure was caused by a read of a variable outside its lifetime
    ```

    正如上面的代码，`const`变量的**初始化器不是常量表达式**，所以不能使用在要求常量表达式的定义里。

    ```CPP
    const int a = 0;
    constexpr int cspa = a; 
    ```

    这个代码就可以成功通过

* `constexpr`函数与模板结合的问题

    ```CPP
    template<typename T>
    constexpr T m_abs(const T t)
    {
      if constexpr(t > 0) //error! expression did not evaluate to a constant. failure was caused by a read of a variable outside its lifetime
        return t;
      else 
        return -t;
    }
    ```

    这个是想要使用`if constexpr`和`constexpr`函数结合，和上一点原因相同，`const`只有初始化器**为常量表达式才是常量表达式**。把`if constexpr`换成`if`即可。

* `constexpr`函数嵌套

    ```CPP
    static constexpr int make_const(const int i) {
        return i;
    }

    constexpr int t1(const int i) {
        return make_const(i);
    }
    ```

    这样嵌套`constexpr`函数就保证了如果实参可以编译期计算，那么就在编译期计算结果。

* `constexpr`函数**不检查**该函数是否真的可以在编译期运行，只有真正在**要求常量表达式的上下文**调用这个函数时，才会检查。事实上，定义`constexpr`函数的要求只有前一节讲的那些。

    ```CPP
    int gloal_var = 0;

    constexpr int change_global_var(void)
    {
        gloal_var = 10;
        return gloal_var;
    }
    ```

    这段代码是合法的，但是`constexpr`函数**修改了生命周期在常量表达式处理过程外的对象*，所以肯定不能在编译器运行。

    事实上，在`MSVC 2019`编译器上，报错为

    ```CPP
    constexpr int test = change_global_var();

    // error C2131: expression did not evaluate to a constant
    // message : failure was caused by non-constant arguments or reference to a non-constant symbol
    // message : see usage of 'gloal_var'
    ```

* 如何定义一个有继承关系的类使其满足`constexpr`变量的要求

    根据`constexpr`变量的要求，这个类必须有一个`constexpr`构造函数，这个构造函数也要满足`constexpr`函数的要求。**注意**，不需要基类有`constexpr`构造函数。当然，如果基类的数据成员**只能通过基类构造函数访问**，那肯定需要基类有`constexpr`构造函数。

    ```CPP
    class Base
    {
    public:
        constexpr Base(int i) : _i(i) {};
    private:
        int _i;
    };

    class Derive : public Base
    {
    public:
        constexpr Derive(int i, int j) :Base(i), _j(j) {};
    private:
        int _j;
    };
    ```

## 如何确定`constexpr`函数在编译期还是运行期运行

简而言之，`constexpr`函数肯定会在编译期运行的条件为

1. **所有的实参均为常量表达式**
2. **结果用在要求常量表达式的地方**

如果不满足第二点，但满足第一点，取决于编译器的实现，决定函数是否在编译期运行。

如果不满足第一点，`constexpr`函数**肯定会在运行期运行**。此时如果把函数用在要求常量表达式的上下文，编译器就会报错。

## 例子

* 使用constexpr函数计算绝对值

```CPP
constexpr int abs_(int x)
{
    if (x > 0) 
    {
        return x;
    } 
    else 
    {
        return -x;
    }
}
```

通常与模板结合起来

```CPP
template<typename T>
constexpr T m_abs(const T t)
{
    if  (t > 0)
        return t;
    else
        return -t;
}
```

* 使用`constexpr`函数计算累加值

```CPP
constexpr int sum(int x)
{
    int result = 0;
    while (x > 0)
    {
        result += x--;
    }
    return result;
}
```

`constexpr`函数里面也是可以使用`while()`的。

* 使用`constexpr`函数修改入参

```CPP
constexpr int next(int x)
{
    return ++x;
}
```

`constexpr`函数可以**修改生命周期和常量表达式相同的对象。**

* 使用`constexpr`成员函数修改成员值

```CPP
#include <iostream>
class X {
public:
	constexpr X() : value(5) {}
	constexpr X(int i) : value(0)
	{
		if (i > 0) 
		{
		    value = 5;
		}
		else 
		{
		    value = 8;
		}
	}
	constexpr void set(int i)
	{
	    value = i;
	}
	constexpr int get() const
	{
	    return value;
	}
private:
	int value;
};
 
constexpr X make_x()
{
	X x;
	x.set(42);
	return x;
}
 
int main()
{
	constexpr X x1(-1);
	constexpr X x2 = make_x();
	constexpr int a1 = x1.get();
	constexpr int a2 = x2.get();
	std::cout << a1 << std::endl;
	std::cout << a2 << std::endl;
}
```

`set()`成员函数修改了成员`value`的值，`constexpr`函数**可以修改生命周期在常量表达式处理过程内的对象**。在处理常量表达式`constexpr X x2 = make_x();`时，在`make_x()`内部定义了一个`X`对象，它的生命周期显然是在这个常量表达式处理过程内的。

注意，不用担心要是`x2.setX(20.0)`怎么办，因为`constexpr`变量隐含`const`不可能调用这个函数。

如果我们对非`constexpr`变量调用`setX()`,那么这个函数肯定不可能在编译期计算出来。

* `constexpr`变量声明必须使用常量表达式初始化，但是，在初始化`constexpr`变量时，却不认为这个变量时`const`的。

```CPP
constexpr int coexp_a = 1;
constexpr int& c = coexp_a; //error! 失去了`const`修饰符
constexpr const int& c = coexp_a; //right!
```

* `auto`推导`constexpr`变量类型时，总是会忽略`constexpr`或者是降级为`const`.

```CPP
constexpr int coexp_a = 1;
auto b =  coexp_a; // type of b is int
auto &c = coexp_a; // type of c is const int &
constexpr auto d =  coexp_a; // type of d is constexpr int
```
