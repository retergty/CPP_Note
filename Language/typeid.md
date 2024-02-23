# typeid

参考文件

* CPP reference[typeid operator](https://en.cppreference.com/w/cpp/language/typeid)
* MSVC 参考手册[typeid](https://learn.microsoft.com/zh-cn/cpp/cpp/typeid-operator?view=msvc-170)

`typeid()`是C++的运算符，可以使用这个运算符得到类型信息，这个运算符会返回`std`库中的`typeinfo`类，具体的类型是`const typeinfo&`。取决于传递给`typeid()`的参数，类型计算可能会发生在编译期或运行期。

## 语法

```Plain
typeid( type )	(1)	
typeid( expression )	(2)	
```

* 当使用`typeid(type)`时，运算符会在编译期计算。

```CPP
#include <typeinfo>

int main()
{
   typeid(int) == typeid(int&); // evaluates to true
}
```

* 当使用`typeid(expression)`时，会根据表达式决定是否在编译期计算，具体参考上面给出的网址。通常来讲对于含有虚函数的类的左值表达式，`typeid()`只能在运行时计算表达式。

注意，`typeid`总是会忽略`cv`限定符，如果可以在编译期计算类型的话，还会忽略引用(包括`&`和`&&`).

## 常用情况

常用的情况就是在模板中使用，输出实例化模板的类型信息

```CPP
// expre_typeid_Operator_3.cpp
// compile with: /c
#include <typeinfo>
template < typename T >
T max( T arg1, T arg2 ) {
   cout << typeid( T ).name() << "s compared." << endl;
   return ( arg1 > arg2 ? arg1 : arg2 );
}
```

使用在模板类函数里，输出当前实例化的类的信息

```CPP
template<typename T,sizr_t M,size_t N>
class Matrix
{
  void print()
  {
    std::cout << "Matrix information: " << typeid(Matrix).name() << std::endl;
  }
}
```
