# reference collapsing

参考文章

* StackOverflow[reference collapsing](https://stackoverflow.com/questions/13725747/what-are-the-reference-collapsing-rules-and-how-are-they-utilized-by-the-c-st)

引用折叠reference collapsing用于完美转发的实现，引用折叠有四种形式

> 1. `A&` `&`折叠为`A&`
> 2. `A&` `&&`折叠为`A&`
> 3. `A&&` `&`折叠为`A&`
> 4. `A&&` `&&`折叠为`A&&`

引用折叠不是意味着我们可以直接写`int & &&`这种形式的代码，编译器会报错，而是在模板参数替换时间接发生。

## 例子

```CPP
template<typename T>
void f(T& t)

f<int&>(a); // f<int&>(int&)
```

由于发生了引用折叠，模板生成的函数签名才会是`f<int&>(int&)`

## 作用在`const`上导致`const`消失

```CPP
template <typename T>
void f3(const T& t) { t = t + 1; }

int main() {
   int a = 3;
   f3<int&>(a);
   ....
}
```

`f3<int&>(a)`的函数原型为`f3(int&)`,`const`消失了。

这是由于`const`修饰的是`T`，等价于`T const &`，但是，引用不能为`const`，所以当模板实参是`int&`时，先去除了`const`再发生引用折叠。
