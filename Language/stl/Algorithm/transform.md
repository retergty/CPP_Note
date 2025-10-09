# transform

定义在头文件`<algorithm>`中，在指定范围内应用函数，并将结果存储至指定的另一个范围内.

参考文档

* [std::transform](https://en.cppreference.com/w/cpp/algorithm/transform.html)

## 声明

```CPP
template< class InputIt, class OutputIt, class UnaryOp >
OutputIt transform( InputIt first1, InputIt last1,
                    OutputIt d_first, UnaryOp unary_op );

template< class ExecutionPolicy,
          class ForwardIt1, class ForwardIt2, class UnaryOp >
ForwardIt2 transform( ExecutionPolicy&& policy,
                      ForwardIt1 first1, ForwardIt1 last1,
                      ForwardIt2 d_first, UnaryOp unary_op );

template< class InputIt1, class InputIt2,
          class OutputIt, class BinaryOp >
OutputIt transform( InputIt1 first1, InputIt1 last1, InputIt2 first2,
                    OutputIt d_first, BinaryOp binary_op );

template< class ExecutionPolicy,
          class ForwardIt1, class ForwardIt2,
          class ForwardIt3, class BinaryOp >
ForwardIt3 transform( ExecutionPolicy&& policy,
                      ForwardIt1 first1, ForwardIt1 last1,
                      ForwardIt2 first2,
                      ForwardIt3 d_first, BinaryOp binary_op );
```

* 1）将函数`unary_op`应用到由`[first1,first2)`指定范围的函数中，将结果存储从`d_first`开始的迭代器中.注意函数运行中不能失效`[first1,first2)`与`d_first`开始的迭代器.也不能修改这些迭代器指定的元素.
* 3）函数`binary_op`接受两个参数，分别由`[first1,last1)`指定的范围与`first2`开始的范围，将结果存储在`d_first`开始的迭代器.注意函数运行中不能失效`[first1,first2)`与`first2`,`d_first`开始的迭代器.也不能修改这些迭代器指定的元素.
