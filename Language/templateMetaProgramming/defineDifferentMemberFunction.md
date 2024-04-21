# 给不同的模板实参定义不同的成员函数

```CPP
template<typename Type>
struct xy_dim
{
    template<typename T = Type, std::enable_if_t<has_const_iterator<T>,bool> = true>
    typename T::value_type first(void) const {
        std::cout << "call  typename T::value_type first(void)";
        return x[0];
    }
    template<typename T = Type, std::enable_if_t<!has_const_iterator<T>,bool> = true>
    T first(void) const {
        std::cout << "call Type first(void) const";
        return x;
    }
    Type x;
    Type y;
};
```

给不同的模板实参定义不同的成员函数，返回值不同，避免了偏特化模板的问题。使用`T=Type`而不是直接使用`Type`的原因就是需要利用模板函数实例化时的`SFINAE`.如果使用了`Type`那么就会在实例化类`xy_dim`时使用实参寻找类型`Type::value_type`，如果没找到，则会报错。（因为替换成员函数模板参数时不是`SFINAE`）。
