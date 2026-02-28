# remove_reference

在头文件`<type_traits>`中定义，移去模板实参的引用类型。

参考文档

* [remove_reference](https://en.cppreference.com/w/cpp/types/remove_reference)

## 类原型

```CPP
template< class T >
struct remove_reference;
```

## 描述

如果`T`是引用类型，将引用删除，否则，依原样返回`T`.

## 成员类定义

* `type`就是去除了引用的`T`.

## 帮助类型

```CPP
template< class T >
using remove_reference_t = typename remove_reference<T>::type;
```

## 可能实现

```CPP
template<class T> struct remove_reference { typedef T type; };
template<class T> struct remove_reference<T&> { typedef T type; };
template<class T> struct remove_reference<T&&> { typedef T type; };
```
