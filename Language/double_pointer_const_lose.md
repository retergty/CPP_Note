# 多重指针下`const`丢失问题

考虑下面的代码

```CPP
const int a = 4;
int b = a;
int* c = &b;
int** d = &c;

const int** e = d;
```

在`MSVC`上编译这段代码，报错为

```Text
error C2440: 'initializing': cannot convert from 'int **' to 'const int **'
message : Conversion loses qualifiers
```

原因是，当我们把`const int** e`初始化为`d`时，`*e`就是`const int*`类型的，所以可以指向`const int`类型。但是,`*d`时`int *`类型的，不能指向`const int`.如果允许这个初始化的话，`**d`就可以改变`const int`的值.

把代码修改为

```CPP
const int a = 4;
int b = a;
int* c = &b;
int** d = &c;

const int*const* e = d;
```

也就是说，底层`const`具有传递性。
