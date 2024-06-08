# target_compile_features

声明目标编译时所需的编译器特性

参考文档

* [target_compile_features](https://cmake.org/cmake/help/latest/command/target_compile_features.html)

## 命令格式

```CMake
target_compile_features(<target> <PRIVATE|PUBLIC|INTERFACE> <feature> [...])
```

## 详细描述

`feature`通常`C/C++`的标准，比如`cxx_std_11`.

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`COMPILE_FEATURES`、`INTERFACE_COMPILE_FEATURES`属性添加新的表项，这三个参数表述了使用这个`target`的别的目标的使用要求。
