# Partial template specialization

参考文章

* CPP reference[Partial template specialization](https://en.cppreference.com/w/cpp/language/partial_specialization)
* CSDN [C++模板全特化（具体化）与偏特化（部分具体化）详解](https://blog.csdn.net/greywolf0824/article/details/106856397)

模板偏特化可以为类模板和变量模板指定一部分模板参数，函数模板**不能**偏特化，但可以重载。

定义模板篇特化前，必须先定义主模板(primary template),可以在任何可以定义主模板的作用域处定义模板偏，比如在类外定义类成员。

```CPP
template<class T1, class T2, int I>
class A {};             // primary template
 
template<class T, int I>
class A<T, T*, I> {};   // #1: partial specialization where T2 is a pointer to T1
 
template<class T, class T2, int I>
class A<T*, T2, I> {};  // #2: partial specialization where T1 is a pointer
 
template<class T>
class A<int, T*, 5> {}; // #3: partial specialization where
                        //     T1 is int, I is 5, and T2 is a pointer
 
template<class X, class T, int I>
class A<X, T*, I> {};   // #4: partial specialization where T2 is a pointer
```

## 语法

```text
template < parameter-list > class-key class-head-name < argument-list > declaration	(1)	
template < parameter-list > decl-specifier-seq declarator < argument-list > initializer(optional) (2)	(since C++14)
```

(1) 是类模板偏特化的语法
(2) 是变量模板偏特化的语法

对于模板偏特化，实参列表`argument-list`有一定的限制

1. 实参不能与主模板完全相同，它必须更加地特例化。

    ```CPP
    template<class T1, class T2, int I> class B {};        // primary template
    template<class X, class Y, int N> class B<X, Y, N> {}; // error
    ```

2. 默认参数不能出现在实参
3. 如果实参是参数包，它必须是最后一个参数
4. 非类型的实参不能特例化一个类型依赖于这个特例化参数的模板参数

    ```CPP
    template<class T, T t> struct C {}; // primary template
    template<class T> struct C<T, 1>;   // error: type of the argument 1 is T,
                                        // which depends on the parameter T
    
    template<int X, int (*array_ptr)[X]> class B {}; // primary template
    int array[5];
    template<int X> class B<X, &array> {}; // error: type of the argument &array is
                                          // int(*)[X], which depends on the parameter 
    ```

    这样就可以

    ```CPP
    template<typename T1,typename T2, T2 t> struct Q {}; // primary template
    template<typename T1> struct Q<T1, int, 1> {};
    ```

## 名称查找

模板特化不会被名称查找发现，只有当主模板被名称查找发现后，它的偏特化才会被考虑.

## 优先级

**类模板**调用优先级：全特化类>偏特化类>主版本模板类；

**变量模板**调用优先级：全特化变量>偏特化变量>主版本模板变量；

**函数模板**调用优先级：常规函数 > 具体化模板函数 > 常规模板函数；

## 调用流程

当类或者变量模板被实例化时，编译器必须去确定是用主模板还是特化模板实例化。

注意，模板实例化发生在模板参数推断后，模板参数推断**只使用主模板**进行参数推断。

1. 如果只有一个特化匹配模板实参，那么直接使用这个特化模板。
2. 如果有超过一个特化匹配模板实参，那么使用最具体化的那个特化，如果有多个特化具体化等级相同，编译器报错。
3. 如果没有特化匹配模板实参，使用主模板。

    ```CPP
    // given the template A as defined above
    A<int, int, 1> a1;   // no specializations match, uses primary template
    A<int, int*, 1> a2;  // uses partial specialization #1 (T = int, I = 1)
    A<int, char*, 5> a3; // uses partial specialization #3, (T = char)
    A<int, char*, 1> a4; // uses partial specialization #4, (X = int, T = char, I = 1)
    A<int*, int*, 2> a5; // error: matches #2 (T = int, T2 = int*, I= 2)
                        //        matches #4 (X = int*, T = int, I = 2)
                        // neither one is more specialized than the other
    ```

寻找匹配的模板特化实际上就是在主模板进行了参数推断后，在每个声明的模板特化都进行一次参数推断，推断模板偏特化为指定的参数，也就是`template<parm>`,寻找是否存在参数集合使得这个偏特化与目前实例化的模板类型相同,如果存在，就进入候选集。比如

```CPP
A<int, int*, 1> a2;  // uses partial specialization #1 (T = int, I = 1)
```

就是在特化

```CPP
template<class T, int I>
class A<T, T*, I> {};   // #1: partial specialization where T2 is a pointer to T1
```

对`A<int,int*,1>`进行参数推断，从而发现这个特化合法。我们也可以这样写偏特化

```CPP
template<int I,class T>
class A<T, T*, I> {};   // #1: partial specialization where T2 is a pointer to T1
```

都表示一个偏特化。

`A`特化比`B`特化更具体就意味着，`A`能接受的类型是`B`的**子集**。

参考文章

* StackOverFlow[What is the partial ordering procedure in template deduction](https://stackoverflow.com/questions/17005985/what-is-the-partial-ordering-procedure-in-template-deduction/17008568#17008568)

## 成员偏特化

显然，对于在偏特化类外声明的成员必须与偏特化具有相同的模板参数(也就是`template<parm>`包括顺序)与相同的模板实参(也就是`A<arg>`)。此外，对于成员偏特化的全特化的成员函数，可以省略`template<>`.

```CPP
template<class T, int I> // primary template
struct A
{
    void f(); // member declaration
};

template<class T, int I>
void A<T, I>::f() {}     // primary template member definition

// partial specialization
template<class T>
struct A<T, 2>
{
    void f();
    void g();
    void h();
    struct B;

    template<typename U>
    void t_f();
};

// member of partial specialization
template<class T>
void A<T, 2>::g() {};

// explicit (full) specialization
// of a member of partial specialization
template<>
void A<char, 2>::h() {};

template<class T>
struct A<T, 2>::B {};

template<>
struct A<char,2>::B {};

template<class T>
template<typename U>
void A<T, 2>::t_f() {};
```

如果主模板和偏特化模板是包括在另一个模板（称作模板`E`）里的，那么，如同其它成员一样，在`E`实例化时，会实例化**所有的声明**（不是定义）。这意味着，如果我们指定了`E`在特定模板下的主模板，编译器就会按照我们指定的生成声明，**忽略偏特化模板**。

```CPP
template<class T> 
struct A // enclosing class template
{
    template<class T2>
    struct B {};      // primary member template
    template<class T2>
    struct B<T2*> {}; // partial specialization of member template
};
 
template<>
template<class T2>
struct A<short>::B {}; // full specialization of primary member template
                       // (will ignore the partial)
 
A<char>::B<int*> abcip;  // uses partial specialization T2=int
A<short>::B<int*> absip; // uses full specialization of the primary (ignores partial)
A<char>::B<int> abci;    // uses primary
```

如同上文的代码例子，由于我们替代编译器指定了`A<short>::B`所以编译器不会隐式继承主模板的偏特化。

## 偏特化不是重载

指定了类或者变量的偏特化**不意味**着我们在实例化类时可以减少输入的实参（那是模板默认参数的功劳），只有通过主模板推断出模板参数类型后，编译器才会考虑偏特化，比如如下的代码

```CPP
template<typename T,typename U = void>
class A {};

template<typename T>
class A<T,int> {};

A<int> a; // use primary A<int,void>
```

当代码中实例化`A<int>`时，由于输入实参过少，使用默认实参,实例化模板`A<int,void>`,偏特化不满足条件。

## 模板偏特化例子

在标准库中，有许多利用模板偏特化实现特定功能的代码,以下举一些例子

* 去除引用的实现

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

当调用`add_lvalue_reference_t<int>`时，`reference_traits`先按照主模板推导为`reference_traits<int,void>`,接着寻找模板偏特化，推断模板偏特化参数`T`,发现`T=int`时模板偏特化为`reference_traits<int, std::void_t<int&>>`，之后再推导`std::void_t<int&>>`类型，发现变为`void`,所以最后偏特化类型与当前实例化的类型相同，使用偏特化的模板。

当调用`add_lvalue_reference_t<void>`时，`reference_traits`先按照主模板推导为`reference_traits<void,void>`,接着寻找模板偏特化，推断模板偏特化参数`T`,发现`T=void`时模板偏特化为`reference_traits<void, std::void_t<void&>>`，代码格式错误，不能给void加上引用，根据`SFINAE`，**剔除**这个偏特化，选择主模板。

* 判断是否是引用的实现

```CPP
// STRUCT TEMPLATE is_reference
template <class>
_INLINE_VAR constexpr bool is_reference_v = false; // determine whether type argument is a reference

template <class _Ty>
_INLINE_VAR constexpr bool is_reference_v<_Ty&> = true;

template <class _Ty>
_INLINE_VAR constexpr bool is_reference_v<_Ty&&> = true;

template <class _Ty>
struct is_reference : bool_constant<is_reference_v<_Ty>> {};
```

使用变量模板，判断类型是否含有引用，当调用`is_reference_v<int&>`时，`is_reference_v`先按照主模板推断为`is_reference_v<int&>`,接着寻找模板偏特化，推断模板偏特化参数`_Ty`,发现`_Ty=int`时模板偏特化为`is_reference_v<int&>`,使用偏特化的模板,从而变量值为`true`.

* 计算阶乘

```CPP
template<int N, typename T = int>
inline constexpr T factorial = N * factorial<N - 1, T>;

template<typename T>
inline constexpr T factorial<1,T> = static_cast<T>(1);
```

使用变量模板，在编译期就可以得到阶乘的值

## 减少输入的模板实参个数

本节讲解一些额外内容，模板偏特化是**无法减少**输入的实参个数的，因为模板参数推断发生在主模板，但是可以通过默认实参或者是`using`减少输入的模板实参个数，用途各不相同。

```CPP
// STRUCT TEMPLATE enable_if
template <bool _Test, class _Ty = void>
struct enable_if {}; // no member "type" when !_Test

template <class _Ty>
struct enable_if<true, _Ty> { // type is _Ty for _Test
    using type = _Ty;
};
template <bool _Test, class _Ty = void>
using enable_if_t = typename enable_if<_Test, _Ty>::type;
```

著名的`enable_if`的`MSVC 2019`实现，便是使用了默认参数减少输入的实参个数，我们可以直接这样使用`typename enable_if<test>::type`,默认实参减少了输入的实参个数。

```CPP
template <bool _Val>
using bool_constant = integral_constant<bool, _Val>;
```

标准库`bool_constant`实际上是`integral_constant`的别名，但是指定了一个模板参数`bool`,这样我们在使用`bool_constant`时，可以这样使用`bool_constant<true>`.
