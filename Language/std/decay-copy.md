# decay-copy

返回`std::forward<T>(value)`(隐式转换为`decay`类型)，一个`value`的`decay`纯右值。这个函数不能使用，是标准库的内部实现，仅作展示用，标准库中也没有`decay-copy`函数（-符号是不合法的）。

参考文档

* [decay-copy](https://en.cppreference.com/w/cpp/standard_library/decay-copy)

## 函数声明

```CPP
template< class T >
typename std::decay<T>::type decay-copy( T&& value );
```

## 可能实现

```CPP
template <class T>
std::decay_t<T> decay_copy(T&& v) { return std::forward<T>(v); }
```

当传递一个引用时，就会在返回值处构造一个新的对象（根据左值引用还是右值引用，使用复制构造或者是移动构造）。
