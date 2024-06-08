# add_definitions

给编译器添加`-D`的预处理器定义

参考文档

* [add_definitions](https://cmake.org/cmake/help/latest/command/add_definitions.html)

## 命令格式

```CMake
add_definitions(-DFOO -DBAR ...)
```

## 详细描述

一般来说，`add_definitions`命令都是给预处理器添加定义，但是它也可以直接给编译器传递参数，只需要不加上`-D`就可以了（已被别的命令代替，**不要使用**）。

当加上`-D`，CMake就会认为是预处理器的定义，从而添加`FOO`到当前目录的`COMPILE_DEFINITIONS`属性中，同时还会添加在当前目录前创建的目标的`COMPILE_DEFINITIONS`属性。

要是不加上`-D`,CMake就会认为是编译器参数，从而添加当前目录的`COMPILE_OPTIONS`属性，同时还会添加在当前目录前创建的目标的`COMPILE_OPTIONS`属性。
