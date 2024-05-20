# delete

析构之前使用`new`所创建的对象并释放`new`所分配的空间，不能用在`placement new`中。

## delete表达式

参考文档

* [delete expression](https://en.cppreference.com/w/cpp/language/delete)

### 语法

```CPP
::(optional) delete   expression
::(optional) delete[] expression	
```

`expression`是可以转换为指针的类型，或者是`prvalue`的指针，指向要销毁的对象。

`1`销毁一个非数组的对象，是由`new`创建的。

`2`销毁一个数组对象，是由`new[]`创建的。

### 描述

设`expression`求值结果为`ptr`.

`1`格式中，`ptr`必须满足

* 是空指针
* 指向`new`创建的非数组的对象
* 指向`new`创建的非数组对象的基类

`2`格式中，`ptr`必须满足

* 是空指针
* 指针的值必须与之前使用`new[]`返回的指针的值相等。

`1`和`2`格式必须与之前使用的`new`表达式所匹配，否则行为未定义。

如果`ptr`不为空，`delete`表达式调用对象的析构函数，如果是数组，为数组的每个元素调用析构函数。所以，在`delete`表达式出现的上下文中，析构函数必须可以访问。

之后，`delete`表达式调用释放函数，对于`1`是`operator delete`，对于`2`是`operator delete[]`，释放所分配的空间。

如果`delete`表达式以`::`开头，就只会在全局作用域查找释放函数。否则，会先进行`ADL`。

如果`ptr`为空，不会调用任何析构函数，但是释放函数可能会调用（未指定），但是默认的释放函数保证在传递空指针时不做任何事情。

如果`ptr`是指向`new`分配对象的基类成员的，基类成员的析构函数必须是`virtual`的。

对于`lambda`表达式，不捕获任何变量必须以括号调用。

```CPP
// delete []{ return new int; }(); // parse error
delete ([]{ return new int; })();  // OK
```
