# target_link_directories

添加对于特定目标编译时库文件查找目录

参考文档

* [target_link_directories](https://cmake.org/cmake/help/latest/command/target_link_directories.html)

## 命令格式

```CMake
target_link_directories(<target> [BEFORE]
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

`target`就是之前通过`add_executable()`或`add_library()`命令创建的目标名。

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`LINK_DIRECTORIES`、`INTERFACE_LINK_DIRECTORIES`属性添加新的表项.如果是`IMPORT`库文件，则只支持`INTERFACE`.

`BRFORE`表示在属性的前面添加对应的表项。

这个命令支持生成期表达式。
