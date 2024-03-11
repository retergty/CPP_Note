# 模板参数推断

参考文章

* CPP reference[Template argument deduction](https://en.cppreference.com/w/cpp/language/template_argument_deduction)

对于`template<typename T> void f(ParamType u)`实例化`f(Arg)`时，其中`ParamType`就是模板类型与`const`，`&`,`&&`，`volatile`,`constexpr`的组合。表达式`Arg`的类型为`A`.

在开始模板参数推断前，会对`P`和`A`做一些处理。

首先，由于`A`是表达式类型，在进行表达式类型分析前，**去除**`A`的引用。包括左值和右值

* 如果`P`不是引用类型，比如不是`T&`

  * `A`执行数组到指针，函数到函数指针的转换。
  * 忽略`A`的顶层`cv`修饰符。

```CPP
template<class T>
void f(T);
 
int a[3];
f(a); // P = T, A = int[3], adjusted to int*: deduced T = int*
 
void b(int);
f(b); // P = T, A = void(int), adjusted to void(*)(int): deduced T = void(*)(int)
 
const int c = 13;
f(c); // P = T, A = const int, adjusted to int: deduced T = int
```

* 如果`P`有`cv`修饰符，忽略`P`的顶层`cv`修饰符
* 如果`P`是引用类型，则去除引用
* 如果`P`是没有`cv`修饰符的右值引用，且对应的实例化函数实参是左值，则给`A`加上左值引用`&`.（`std::forward`就是利用了这一点实现的）。注意，如果是进行类模板实参推断，不处理这个情况。

```CPP
template<class T>
int f(T&&);       // P is an rvalue reference to cv-unqualified T (forwarding reference)
 
template<class T>
int g(const T&&); // P is an rvalue reference to cv-qualified T (not special)
 
int main()
{
    int i;
    int n1 = f(i); // argument is lvalue: calls f<int&>(int&) (special case)
    int n2 = f(0); // argument is not lvalue: calls f<int>(int&&)
 
//  int n3 = g(i); // error: deduces to g<int>(const int&&), which
                   // cannot bind an rvalue reference to an lvalue
}
```

处理完毕后，才会开始进行模板参数推断。模板参数推断就是找到一个类型，使得用这个类型替换`T`后，推导出的`A`(`deduced A`)(就是经过上面的处理，并用推导出的类型替换`T`的`P`)与经过修改的`A`(`transformed A`)(就是经过上面的处理`A`)**相同**。

如果找不到这个类型，那么以下的例外条件也可以接受

1. 如果`P`是引用类型，推导出的`A`可以加上`cv`限定符。

      ```CPP
      template<typename T>
      void f(const T& t);
      
      bool a = false;
      f(a); // P = const T&, adjusted to const T, A = bool:
            // deduced T = bool, deduced A = const bool
            // deduced A is more cv-qualified than A
      ```

2. 经过修改的`A`是指针，且可以进行`const`转换为推导出的`A`.

      ```CPP
      template<typename T>
      void f(const T*);
      
      int* p;
      f(p); // P = const T*, A = int*:
            // deduced T = int, deduced A = const int*
            // qualification conversion applies (from int* to const int*)
      ```

3. 如果`P`是类，且经过修改的`A`可以是推导出的`A`的派生类，指针也可以。

      ```CPP
      template<class T>
      struct B {};
      
      template<class T>
      struct D : public B<T> {};
      
      template<class T>
      void f(B<T>&) {}
      
      void f()
      {
      D<int> d;
      f(d); // P = B<T>&, adjusted to P = B<T> (a simple-template-id), A = D<int>:
            // deduced T = int, deduced A = B<int>
            // A is derived from deduced A
      }
      ```

## 例子

```CPP
template<typename T>
void f(T t);

int a = 0;
int &b = a;
f(a);
f(b);
```

两个模板函数调用都实例化`f<int>(int)`。

## 注意点

* 模板参数推断只是推断出模板参数，但不意味着这个调用就是合法的，合不合法之后判断。
