# target_compile_definitions

给特定目标添加预处理器定义

参考文档

* [target_compile_definitions](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)

## 命令格式

```CMake
target_compile_definitions(<target>
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

给特定目标添加预处理器定义，会直接影响编译特定目标时编译器的行为。

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`COMPILE_DEFINITIONS`、`INTERFACE_COMPILE_DEFINITIONS`属性添加新的表项，这三个参数表述了使用这个`target`的别的目标的使用要求。
