# if constexpr

参考文章

* CPP reference[if statement](https://en.cppreference.com/w/cpp/language/if)
* StackOverFlow[Difference between "if constexpr()" Vs "if()"](https://stackoverflow.com/questions/43434491/difference-between-if-constexpr-vs-if)

`if constexpr`又叫做编译期`if`，顾名思义，编译器是在编译期处理这个`if`语句的。通常是在模板元编程使用，可以实现不同的模板实参对应者不同的行为，并且不添加运行期开销。

## 语法

```CPP
attr(optional) if constexpr(optional) ( init-statement(optional) condition ) statement-true	(1)	
attr(optional) if constexpr(optional) ( init-statement(optional) condition ) statement-true else statement-false	(2)
```

`condition`必须是可以转换为`bool`值的**常量表达式**。如果`condition`为真，那么编译期丢弃`statement-false`部分，反之亦然。

废弃语句中的`return`**不会**参与函数返回值推断。

```CPP
template<typename T>
auto get_value(T t)
{
    if constexpr (std::is_pointer_v<T>)
        return *t; // deduces return type to int for T = int*
    else
        return t;  // deduces return type to int for T = int
}
```

废弃语句中可以使用未定义的变量和函数，因为解决未定义的变量可以发生在链接期

```CPP
extern int x; // no definition of x required
 
int f()
{
    if constexpr (true)
        return 0;
    else if (x)
        return x;
    else
        return -x;
}
```

但是在模板以外，哪怕是丢弃的语句也会进行充分的语法检查，

```CPP
void f()
{
    if constexpr(false)
    {
        int i = 0;
        int *p = i; // Error even though in discarded statement
    }
}
```

如果在模板里使用`if constexpr`,且当模板实例化时，`condition`可以直接得出，那么被丢弃的部分不会进行实例化。

```CPP
template<typename T, typename ... Rest>
void g(T&& p, Rest&& ...rs)
{
    // ... handle p
    if constexpr (sizeof...(rs) > 0)
        g(rs...); // never instantiated with an empty argument list.
}
```

```CPP
template<class T>
void g()
{
    auto lm = [=](auto p)
    {
        if constexpr (sizeof(T) == 1 && sizeof p == 1)
        {
            // this condition remains value-dependent after instantiation of g<T>,
            // which affects implicit lambda captures
            // this compound statement may be discarded only after
            // instantiation of the lambda body
        }
    };
}
```

但是，被丢弃的部分并不是和`#if`预处理语句一样，被丢弃的部分也会进行**充分的语法检查**，当然，如果依赖于模板参数，那么在实例化时判断`if constexpr`，错误部分被丢弃，不会做依赖于模板参数的检查。

换句话说，被丢弃的部分不能对每一个模板特例都是语法错误。

```CPP
template<typename T>
void f()
{
    if constexpr (std::is_arithmetic_v<T>)
        // ...
    else {
        using invalid_array = int[-1]; // ill-formed: invalid for every T
        static_assert(false, "Must be arithmetic"); // ill-formed before CWG2518
    }
}
```

如上的代码是不能被编译的，语法错误。

为了消除`static_assert`问题，我们可以让它依赖于模板参数

```CPP
template<typename>
inline constexpr bool dependent_false_v = false;
 
template<typename T>
void f()
{
    if constexpr (std::is_arithmetic_v<T>)
        // ...
    else {
        // workaround before CWG2518
        static_assert(dependent_false_v<T>, "Must be arithmetic");
    }
}
```
