# for_each

定义在头文件`algorithm`中，在指定的迭代器范围（左闭右开）中，为每个迭代器解引用并调用函数对象`f`.

参考文档

* [std::for_each](https://en.cppreference.com/w/cpp/algorithm/for_each)

## 声明

```CPP
template< class InputIt, class UnaryFunc >
UnaryFunc for_each( InputIt first, InputIt last, UnaryFunc f );

template< class ExecutionPolicy, class ForwardIt, class UnaryFunc >
void for_each( ExecutionPolicy&& policy,

               ForwardIt first, ForwardIt last, UnaryFunc f );
```

在指定的迭代器范围（左闭右开）中，为每个迭代器解引用并调用函数对象`f`.如果`f`有返回值，这个返回值会被忽略.

`f`必须是复制可构造的.类型必须是

```CPP
void fun(const Type &a);
```

其中`Type`就是迭代器解引用后可以隐式转换为的类型.
