# target_compile_options

为特定目标添加编译器编译时的参数

参考文档

* [target_compile_options](https://cmake.org/cmake/help/latest/command/target_compile_options.html)

## 命令格式

```CMake
target_compile_options(<target> [BEFORE]
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

为特定目标添加编译器编译时的参数,会直接影响指定目标编译时传递给编译器的参数。

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`COMPILE_OPTIONS`、`INTERFACE_COMPILE_OPTIONS`属性添加新的表项，这三个参数表述了使用这个`target`的别的目标的使用要求。

`BEFORE`表示添加到目标对应属性的前面，要是没有使用这个参数，则默认添加到后面。

这个命令支持生成期表达式。
