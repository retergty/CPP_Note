# bind

定义在头文件`<functional>`中，生成一个新的函数对象适应原函数对象的参数列表。

参考文档

* [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

## 函数声明

```CPP
template< class F, class... Args >
/* unspecified */ bind( F&& f, Args&&... args );
template< class R, class F, class... Args >
/* unspecified */ bind( F&& f, Args&&... args );
```

* `f`是可调用对象，比如函数对象，函数指针，函数引用，指向成员函数的指针，指向成员的指针。
* `args`是绑定的参数，注意，不需要绑定`f`的所有参数,但是需要使用`std::placeholders`占位
* 返回值`g`可以用`std::function`接受，参数就是`f`未绑定的参数。

## 描述

`std::bind`函数模板为函数`f`生成一个函数包装器，调用这个包装器等价于调用函数`f`,参数`args`.

如果`std::is_constructible<std::decay<F>::type, F>::value`或者`std::is_constructible<std::decay<Arg_i>::type, Arg_i>::value`为假，程序不正确。

如果函数或函数的参数不是默认可构造的，或者是可析构的，程序行为未定义。

对于指向成员函数的指针，或者是指向成员的指针，第一个参数必须是指向拥有这个成员对象的引用或者是指针。

`args`是被复制或者是移动的，永远不会按照引用传递，除非使用了引用包装器`std::ref`或`std::cref`.

`args`可以是`std::placeholders`中的`_1`,`_2`表示未绑定的参数。

`std::placeholers`中的`_1`,`_2`指的是`std::bind`返回的函数对象的参数列表中的第一个，第二个参数，不是原来函数`f`的参数列表中的第一个，第二个参数。

## 例子

### 绑定普通函数

```CPP
double callableFunc (double x, double y) {return x/y;}
auto NewCallable = std::bind (callableFunc, std::placeholders::_1,2);  
std::cout << NewCallable (10) << '\n';                       
```

### 绑定成员函数

```CPP
class Base
{
public:
    void display_sum(int a1, int a2)
    {
        std::cout << a1 + a2 << '\n';
    }
 
    int m_data = 30;
};
int main() 
{
    Base base;
    auto newiFunc = std::bind(&Base::display_sum, &base, 100, std::placeholders::_1);
    newiFunc(20); // should out put 120. 
}
```

### 绑定引用参数

```CPP
ostringstream os1;
// ostream不能拷贝，若希望传递给bind一个对象，
// 而不拷贝它，就必须使用标准库提供的ref函数
for_each(words.begin(), words.end(),
          bind(printInfo, ref(os1), _1, c));
```
