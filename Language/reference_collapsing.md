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
