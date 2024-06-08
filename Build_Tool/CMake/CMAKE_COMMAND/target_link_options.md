# target_link_options

添加链接时传递给链接器的参数，只对可执行文件，共享库，模块有效果，对静态库没有效果。

参考文档

* [target_link_options](https://cmake.org/cmake/help/latest/command/target_link_options.html)

## 命令格式

```CMake
target_link_options(<target> [BEFORE]
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

`target`就是之前通过`add_executable()`或`add_library()`命令创建的目标名。

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`LINK_OPTIONS`、`INTERFACE_LINK_OPTIONS`属性添加新的表项.如果是`IMPORT`库文件，则只支持`INTERFACE`.

`BRFORE`表示在属性的前面添加对应的表项。

这个命令支持生成期表达式。
