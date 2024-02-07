# SFINAE

参考文件

* CPP reference[SFINAE](https://en.cppreference.com/w/cpp/language/sfinae)

模板替换失败不是错误(Substitution Failure Is Not An Error).这个规则应用在函数重载决议中，当替换显式声明或者编译器推导的模板类型失败后，就会从重载集合中丢弃这个特化函数，不引发编译错误。

这个特性会用于模板元编程中。

## 解释

函数模板参数(Function template parameters)被模板实参(template arguments)替换会发生两次

* 显式指定的模板参数会在模板参数推断前被替换
* 推断出的模板参数和默认参数会在模板推断后被替换

类型替换发生在

* 函数类型中使用的类型，也就函数的返回值类型，参数的类型。
* 模板参数声明中使用的类型。
* 部分特化列表的模板实参中使用的类型。
* 函数类型中使用的表达式
* 模板参数声明中使用的表达式
* 部分特化列表的模板实参中使用的表达式。
* 在`explicit`中使用的表达式

那么，模板替换失败指的就是如果使用模板实参替换后，上述提到的类型或者是表达式出现格式错误(ill-formed)。

## SFINAE例子与情况

参见`CPP reference`[SFINAE](https://en.cppreference.com/w/cpp/language/sfinae)

## 模板偏特化下的SFINAE

对于模板偏特化下，替换模板参数如果产生了错误，那么就认为是SFINAE，此时会直接移除这个模板偏特化，就好像是在函数重载决议中一样。

```CPP
// primary template handles non-referenceable types:
template<class T, class = void>
struct reference_traits
{
    using add_lref = T;
    using add_rref = T;
};
 
// specialization recognizes referenceable types:
template<class T>
struct reference_traits<T, std::void_t<T&>>
{
    using add_lref = T&;
    using add_rref = T&&;
};
 
template<class T>
using add_lvalue_reference_t = typename reference_traits<T>::add_lref;
 
template<class T>
using add_rvalue_reference_t = typename reference_traits<T>::add_rref;
```

## 区别SFINAE

对于模板类，类的模板参数会在类实例化时替换为模板实参，所以在模板类的成员函数里使用SFINAE需要额外定义一个新的模板参数，否则不会认为是SFINAE。

```CPP
template<typename Type,size_t M,size_t N>
class Matrix
{
public:
    Matrix()
    {
        memset(_data,0, M * N * sizeof(Type));
    }
    void slice(void)
    {
        Slice<Type, M, N> sli;
        sli.print();
    }

    template<size_t U = M>
    Type& operator()(std::enable_if_t<(U == 1),size_t> i)
    {
        std::cout << "SFINAE 1" << std::endl;
        return _data[0][i];
    }

    template<size_t U = M>
    Type& operator()(std::enable_if_t<(U != 1), size_t> i)
    {
        std::cout << "SFINAE 2" << std::endl;
        return _data[0][i];
    }

private:
    Type _data[M][N];
};
```

比如对于`Martrix`类，希望使用`enable_if`来实现当`M==1`是和`M!=1`时的调用表达式的不同。必须要定义`template<size_t U = M>`这样在模板函数替换时，才会认为是SFINAE。

如果把`U`改为`M`,`M`是在类实例化时被替换，此时由于模板类成员函数的特性（用到才实例化），对应的函数不会实例化.所以当使用到`operator()`时`M`已经被替换了，此时的错误不是SFINAE。

使用`msvc`报错信息为

```shell
error C2938: 'std::enable_if_t<false,size_t>' : Failed to specialize alias template
```

## 不是SFINAE

```CPP
#include <iostream>
#include <type_traits>

#define E(expr) template<bool Y = true, typename std::enable_if_t<(expr) && Y, int> = 0>

template<typename T, int N>
struct Vec
{
    static constexpr bool EXPR = std::is_same<float, T>::value && N == 4;
    
    int m_x;
    
    E(EXPR)
    void foo()
    {
        m_x = 1;
    }
    
    E(!EXPR)
    void foo()
    {
        m_x = 2;
    }
};

int main()
{
    Vec<float, 3> v;
    v.foo();
    std::cout << v.m_x << std::endl;
}
```

对于`gcc`和`clang`编译成功，但是对于`msvc`编译不成功，原因就是，这个错误不是SFINAE。

`c++`标准允许要是一个模板的每个特化都是格式错误`ill-formed`，可以生成一个错误，对于上述代码，由于`EXPR`在类模板函数实例化时已经获得了`false`，实际上就是`template<bool Y = true, typename std::enable_if_t<false && Y, int> = 0>`。此时取决于编译器的实现，对于`enable_if_t`在特化前就是格式错误的了，还没有进行模板实参替换，所以还没到SFINAE.但是`gcc`不认为这个是一个错误。
