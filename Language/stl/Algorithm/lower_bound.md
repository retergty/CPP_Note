# lower_bound

定义在头文件`algorithm`中，在指定的迭代器范围（左闭右开）中使用二分查找法第一个不小于指定值的元素.

参考文档

* [std::upper_bound](https://en.cppreference.com/w/cpp/algorithm/upper_bound.html)

## 声明

```CPP
template< class ForwardIt, class T >
ForwardIt lower_bound( ForwardIt first, ForwardIt last,
                       const T& value );
template< class ForwardIt, class T, class Compare >
ForwardIt lower_bound( ForwardIt first, ForwardIt last,
                       const T& value, Compare comp );
```

在指定的迭代器范围（左闭右开）中使用二分查找法第一个不小于指定值的元素.`[first,last)`在比较函数意义下是从小到大的.

（1）使用`operator<`比较

（2）使用自定义的比较器`comp`比较

如果在`[first,last)`中有这个元素，返回指向这个元素的迭代器，否则返回`last`.

如果`[first,last)`中的元素没有按照指定比较方法分好区（排好序），函数未定义.
