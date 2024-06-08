# add_compile_definitions

添加编译器预处理器的定义

参考文档

* [add_compile_definitions](https://cmake.org/cmake/help/latest/command/add_compile_definitions.html#command:add_compile_definitions)

## 命令格式

```CMake
add_compile_definitions(<definition> ...)
```

## 详细描述

添加`<definition>`到当前目录的`COMPILE_DEFINITIONS`属性中，同时还会添加在当前目录前创建的目标的`COMPILE_DEFINITIONS`属性。

`<definition>`使用格式`VAR`或者是`VAR=value`.

这个命令支持编译期表达式。
