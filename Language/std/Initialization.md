# Initialization

`C++`初始化分为复制初始化，直接初始化和列表初始化三种方式。

参考文档

* [CPP Reference - Initialization](https://en.cppreference.com/w/cpp/language/initialization)

## 复制初始化

复制初始化的语法如下：

```CPP
T object = value;
```

## 直接初始化

直接初始化的语法如下：

```CPP
T object(value);
```

## 列表初始化

列表初始化的语法如下：

```CPP
T object{value};
```

## 不同点

* 直接初始化不会匹配形如`T(std::initializer_list<T>)`的构造函数，而列表初始化会。
* 列表初始化不允许窄化转换，而复制初始化和直接初始化允许。
* 如果形如`T t()`此时需要使用花括号初始化来避免被解析为函数声明。
