# reference_wrapper

定义在头文件`<functional>`中，是引用的包装器。

参考文档

* [reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper)

## 类原型

```CPP
template< class T >
class reference_wrapper;
```

## 描述

`std::reference_wrapper`是一个通用的引用包装器，可以使得引用变为可复制和可赋值的，也就是说，复制和赋值不会丢失对原本对象的引用。

`reference_wrapper`是复制可构造(CopyConstructible),复制可赋值(CopyAssignable)的，`reference_wrapper`的实例是一个类，但是可以隐式转换为`T&`,所以可以用作接受`T&`的函数的参数。

如果`T`是可调用的，则`reference_wrapper`也是可调用的。

`reference_wrapper`保证平凡可复制

## 成员类定义

* `type`就是`T`.

## 成员函数

### 构造

* [reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper)

  ```CPP
  template< class U >
  reference_wrapper( U&& x ) noexcept(/*see below*/) ;
  reference_wrapper( const reference_wrapper& other ) noexcept;
  ```

  `1`把`x`转换为`T&`，如同`T& t = std::forward<U>(x);`,并存储引用。

### 获取引用

* [get,operator T&](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper/get)

  ```CPP
  operator T& () const noexcept;
  T& get() const noexcept;
  ```

  返回所包装的引用。

* [operator()](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper/operator())

  ```CPP
  template< class... ArgTypes >
  std::invoke_result_t<T&, ArgTypes...>
    operator() ( ArgTypes&&... args ) const noexcept;
  ```

  调用包装的可调用对象，如同`INVOKE(get(), std::forward<ArgTypes>(args)...)`.这个函数只有在所包装的引用是可调用对象时才起作用。

## 用途

* 用在`thread`构造函数中，用于向新的线程传递引用。

## 帮助函数ref与cref

```CPP
template< class T >
std::reference_wrapper<T> ref( T& t ) noexcept;
template< class T >
std::reference_wrapper<T>
    ref( std::reference_wrapper<T> t ) noexcept;
template< class T >
void ref( const T&& ) = delete;

template< class T >
std::reference_wrapper<const T> cref( const T& t ) noexcept;
template< class T >
std::reference_wrapper<const T>
    cref( std::reference_wrapper<T> t ) noexcept;
template< class T >
void cref( const T&& ) = delete;
```

可以使用帮助函数轻松地创建引用包装器，同时自动进行实参推导。

## 可能实现

```CPP
namespace detail
{
    template<class T> constexpr T& FUN(T& t) noexcept { return t; }
    template<class T> void FUN(T&&) = delete;
}
 
template<class T>
class reference_wrapper
{
public:
    // types
    using type = T;
 
    // construct/copy/destroy
    template<class U, class = decltype(
        detail::FUN<T>(std::declval<U>()),
        std::enable_if_t<!std::is_same_v<reference_wrapper, std::remove_cvref_t<U>>>()
    )>
    constexpr reference_wrapper(U&& u)
        noexcept(noexcept(detail::FUN<T>(std::forward<U>(u))))
        : _ptr(std::addressof(detail::FUN<T>(std::forward<U>(u)))) {}
 
    reference_wrapper(const reference_wrapper&) noexcept = default;
 
    // assignment
    reference_wrapper& operator=(const reference_wrapper& x) noexcept = default;
 
    // access
    constexpr operator T& () const noexcept { return *_ptr; }
    constexpr T& get() const noexcept { return *_ptr; }
 
    template<class... ArgTypes>
    constexpr std::invoke_result_t<T&, ArgTypes...>
        operator() (ArgTypes&&... args ) const
            noexcept(std::is_nothrow_invocable_v<T&, ArgTypes...>)
    {
        return std::invoke(get(), std::forward<ArgTypes>(args)...);
    }
 
private:
    T* _ptr;
};
 
// deduction guides
template<class T>
reference_wrapper(T&) -> reference_wrapper<T>;
```
