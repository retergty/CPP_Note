# forward

在头文件`utility`中定义，用于模板实参完美转发。

参考文件

* CPP reference [std::forward](https://en.cppreference.com/w/cpp/utility/forward)
* stackoverflow [How does std::forward receive the correct argument?](https://stackoverflow.com/questions/29135698/how-does-stdforward-receive-the-correct-argument)

## 语法

```CPP
template< class T >
constexpr T&& forward( std::remove_reference_t<T>& t ) noexcept;
template< class T >
constexpr T&& forward( std::remove_reference_t<T>&& t ) noexcept;
```

`MSVC 2019`的实现为

```CPP
// FUNCTION TEMPLATE forward
template <class _Ty>
_NODISCARD constexpr _Ty&& forward(
    remove_reference_t<_Ty>& _Arg) noexcept { // forward an lvalue as either an lvalue or an rvalue
    return static_cast<_Ty&&>(_Arg);
}

template <class _Ty>
_NODISCARD constexpr _Ty&& forward(remove_reference_t<_Ty>&& _Arg) noexcept { // forward an rvalue as an rvalue
    static_assert(!is_lvalue_reference_v<_Ty>, "bad forward call");
    return static_cast<_Ty&&>(_Arg);
}
```

## 用法

第一个重载是按照模板实参将左值转化为左值或者是右值，如果模板实参是引用类型，比如`int&`那么就还是以左值传递；如果模板实参不是引用类型，比如`int`那么就把左值以右值传递。

第二个重载是把右值以右值传递，同时避免右值以左值传递。

第一个重载用在如下代码

```CPP
template<typename T>
void foo(T&);

template<typename T>
void foo(T&&);

template<typnename T>
void wrapper(T&& arg)
{
    // arg is always lvalue
    foo(std::forward<T>(arg)); // Forward as lvalue or as rvalue, depending on T
}
```

注意，哪怕我们是用右值调用的`wrapper`但是由于**变量名是左值**，所以给`foo(arg)`会调用`foo(T&)`函数，这与我们传递右值不符。所以我们使用`std::forward<T>(arg)`。这样，如果是右值，那么还是右值传递，如果是左值，那么也还是左值传递。

第二个重载用在如下代码

```CPP
struct Arg
{
    int i = 1;
    int  get() && { return i; } // call to this overload is rvalue
    int& get() &  { return i; } // call to this overload is lvalue
};

// transforming wrapper
template<class T>
void wrapper(T&& arg)
{
    foo(std::forward<decltype(std::forward<T>(arg).get())>(std::forward<T>(arg).get()));
}
```

这里使用了三个`std::forward`以下分别讲述。

`std::forward<T>(arg).get()`和第一个重载想法一样，不确定`arg`是左值还是右值，但是要求保留`arg`的左右值性。由于**变量名是左值**，单纯使用`arg.get()`只会调用`int& get() &`函数。

`decltype(std::forward<T>(arg).get())`取得返回值类型，用于下一个`std::forward`.

最外层的`std::forward<decltype(std::forward<T>(arg).get())>(std::forward<T>(arg).get())`就是综合了两个重载，如果`arg`是左值，那么这个变为`arg.get()`。如果`arg`是右值，那么还是以右值传递。

咋一看这个和`std::forward<T>(arg).get()`结果相同，是的，在这个设计下条件是相同的。但是，如果产生了设计缺陷，比如`int&  get() &&`，那么使用上面的就可以很好的解决。也就是说，外层的`forward`作用为，避免右值处理后变为左值传递。

## 常用地方

通常使用在我们不知道参数是左值还是右值，但是需要保持参数的左右值性。通常只有一种情况我们不知道参数的左右值性，也就是在**万能引用**中。其他情况`std::move`可能是更好的方法。
