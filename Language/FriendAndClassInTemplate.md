# friend and class in template

本文讨论当涉及到模板时，在类里声明友元的不同情况。

参考文章

* [Friends and templates (C++ only)](https://www.ibm.com/docs/en/zos/2.4.0?topic=only-friends-templates-c)

模板类中声明友元有四种关系

* 一对多，一个非模板的友元是多个实例化模板的友元。
* 多对一，多个实例化的模板友元只是一个特定的非模板类的友元。
* 一对一，一个模板的友元是一个对应的模板类的友元。
* 多对多，多个实例化的模板友元是多个实例化模板的友元。

## 例子

以下的代码段说明了这四种关系

```CPP
class B{
   template<class V> friend int j();
}

template<class S> g();

template<class T> 
class A {
   friend int e();
   friend int f(T);
   friend int g<T>();
   template<class U> friend int h();
};
```

函数`e()`是一对多的关系，同一个函数`e()`是所有实例化的`A`的友元。

函数`f()`是一对一的关系，但是不推荐这么写，因为如果实例化的模板的参数没有匹配找到对应的`f()`的定义，就会报错。

函数`g()`也是一对一的关系，一个实例化的`A`只有一个对应的实例化的`g()`的友元。**注意**!函数`g()`要预先声明。

函数`h()`是多对多的关系，所有实例化的模板函数`h()`都是所有实例化的模板`A`的友元。

函数`j()`是多对一的关系，所有实例化的模板函数`h()`是非模板类`B`的友元。
