# 重载决议

函数重载决议的关键就是寻找最匹配的函数，也就是最具体的函数，如果两个函数都不比对方更具体，则重载决议失败。

## 例子

### 匹配左值和右值

```CPP
void Fun(const int&);
void Fun(int &&);

int i = 0;
const int ci = 0;
Fun(0);  //call void Fun(int &&);
Fun(i); //call void Fun(const int&);
Fun(ci); //call void Fun(const int&);
Fun(std::move(i)); //call void Fun(int &&);
Fun(std::move(ci)); //call void Fun(const int&);
```

前面四个函数匹配都很显然，但是第五个匹配匹配的是函数`void Fun(const int&);`，因为`std::move(ci)`的类型是`const int &&`,`const int&`可以指向右值，`int &&`不能绑定到`const`引用上。

```CPP
void Fun(const int&);
void Fun(const int &&);

int i = 0;
const int ci = 0;
Fun(0);  //call void Fun(const int &&);
Fun(i); //call void Fun(const int&);
Fun(ci); //call void Fun(const int&);
Fun(std::move(i)); //call void Fun(const int &&);
Fun(std::move(ci)); //call void Fun(const int &&);
```

注意函数`Fun(std::move(i))`的`std::move(i)`的类型是`int &&`，两个函数都要执行`const`转换，转换完毕后，`void Fun(const int &&);`更具体，所以匹配到。

分析上述函数匹配结果，发现`void Fun(int &&);`在`Fun(std::move(ci));`调用时，把右值绑定到了常左值引用上。而`void Fun(const int &&);`则没有。

为了避免函数绑定到右值上，我们可以

```CPP
void Fun(const int&);
void Fun(const int &&) = delete;
```

经典的赋值运算符重载

```CPP
class T
{
  T& operator=(const T&);
  T& operator=(T&&);
}
```

为什么不用`T& operator=(const T&&)`这是因为，右值赋值后，我们可能要修改被移动的对象，使得他安全析构，进一步使用。
