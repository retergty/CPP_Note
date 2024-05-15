# shared_ptr

参考文档

* CPP Reference[shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)

定义在头文件`<memory>`中，是动态内存管理常用到的类对象。

## 类原型

```CPP
template< class T > class shared_ptr;
```

## 描述

`std::shared_ptr`是一个行为和指针十分类似的类，它通过指针取得动态分配的对象的所有权。多个`shared_ptr`可以指向同一个对象。

由`shared_ptr`所拥有的对象当如下情况满足时被销毁，并释放其所占有的动态内存。

* 最后一个拥有这个对象的`shared_ptr`被销毁
* 最后一个拥有这个对象的`shared_ptr`被分配给另一个指针通过函数`operator=`或`reset()`.

动态分配的对象是使用`delete`或者是用户自定义的删除器销毁的。

`shared_ptr`可以在拥有一个对象的所有权的同时，存储指向另外一个对象的指针。这个特性可以用于指向成员对象的同时，拥有整个对象的所有权。存储的指针就是使用`get()`,解引用运算符，比较运算符使用的指针，而拥有所有权的指针就是指向被销毁对象的指针。

当`shared_ptr`不拥有任何对象的所有权时，它就被认为是空的。

所有的`shared_ptr`都是`复制可构造(CopyConstructible)`，`复制可赋值(CopyAssignable)`，以及可`小于比较(LessThanComparable)`的。

`shared_ptr`**所有的**成员函数，都可以在不同线程的不同的`shared_ptr`对象上调用，不需要额外的同步操作，哪怕这些`shared_ptr`对象是一个`shared_ptr`的复制，且拥有同一个对象的所有权。但是，在不同线程操作同一个`shared_ptr`对象会带来竞争，`std::atomic<shared_ptr>`可以避免这个事情发生。

## 成员类型定义

* `element_type`就是`std::remove_extent_t<T>`.
* `weak_type`就是`std::weak_ptr<T>`

## 成员函数

### 构建对象

* [shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr)

  可以从指针构建`shared_ptr`,也可以指定用户定义的删除器，接管之前指针管理的动态内存对象。