# 构造函数笔记

## 构造函数的值类型

根据`CPP Reference`,构造函数在`C++`中被认为是**类型转换表达式**(cast expression),所以根据值定义，构造函数表达式的值类型是`prvalue`.所以，`non-const`引用无法绑定在构造函数表达式上。
