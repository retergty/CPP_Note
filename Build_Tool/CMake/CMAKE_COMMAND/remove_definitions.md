# remove_definitions

移去之前以`add_definitions`创建的定义。

参考文档

* [add_definitions](https://cmake.org/cmake/help/latest/command/remove_definitions.html)

## 命令格式

```CMake
remove_definitions(-DFOO -DBAR ...)
```

## 详细描述

当加上`-D`，CMake就会认为是预处理器的定义，从而去除当前目录的`COMPILE_DEFINITIONS`属性的`FOO`表项，同时还会去除在当前目录前创建的目标的`COMPILE_DEFINITIONS`属性的`FOO`表项。

当不加上`-D`,CMake就会认为是编译器参数，从而去除当前目录的`COMPILE_OPTIONS`属性的对应表项，同时还会去除在当前目录前创建的目标的`COMPILE_OPTIONS`属性的对应表项。
