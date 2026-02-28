# decay

定义在头文件`<type_traits>`中，如同值传递给函数的实参一样进行类型转换。

参考文档

* [decay](https://en.cppreference.com/w/cpp/types/decay)

## 类原型

```CPP
template< class T >
struct decay;
```

## 描述

如同传递给函数的实参一样进行类型转换，也就是说

* 如果`T`是`U`的数组类型，或者是指向它的引用类型，则转换为`U*`(数组劣化).
* 否则，如果`T`是函数类型`F`或者是指向它的引用类型，则转换为`std::add_pointer<F>::type`(函数指针自动转化).
* 否则，转换为`std::remove_cv<std::remove_reference<T>::type>::type`.

## 成员类定义

* `type`就是进行了类型转换后的`T`。

## 帮助类型

```CPP
template< class T >
using decay_t = typename decay<T>::type;
```

## 可能实现

```CPP
template<class T>
struct decay
{
private:
    typedef typename std::remove_reference<T>::type U;
public:
    typedef typename std::conditional< 
        std::is_array<U>::value,
        typename std::add_pointer<typename std::remove_extent<U>::type>::type,
        typename std::conditional< 
            std::is_function<U>::value,
            typename std::add_pointer<U>::type,
            typename std::remove_cv<U>::type
        >::type
    >::type type;
};
```
