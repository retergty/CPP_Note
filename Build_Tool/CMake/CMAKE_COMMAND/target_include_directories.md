# target_include_directories

给特定目标添加头文件搜索路径

参考文档

* [target_include_directories](https://cmake.org/cmake/help/latest/command/target_include_directories.html)

## 命令格式

```CMake
target_include_directories(<target> [SYSTEM] [AFTER|BEFORE]
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

给特定目标添加头文件搜索路径，`target`就是先前通过`add_executable()`或者`add_library()`创建的目标。

`[AFTER|BEFORE]`表示是添加到对应属性的后面还是前面，默认是后面。

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`INCLUDE_DIRECTORIES`、`INTERFACE_INCLUDE_DIRECTORIES`属性添加新的表项，这三个参数表述了使用这个`target`的别的目标的使用要求。

`SYSTEM`告诉某些平台的编译器这个目录是系统的头文件的搜索目录(system include directories)，这个参数的实际效果需要参考编译器文档。

要是`SYSTEM`与`PUBLIC`或`INTERFACE`一起使用，那么还会添加对应的值到`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`目标属性。

`item`可以是绝对路径或者是相对路径，相对路径相对于当前的源文件目录，CMake会转换为绝对路径存储。

这个命令支持生成期表达式。

## 常用例子

有时，对于构建和安装使用不同的头文件搜索路径，此时可以使用生成期表达式。

```CMake
target_include_directories(mylib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mylib>
  $<INSTALL_INTERFACE:include/mylib>  # <prefix>/include/mylib
)
```

相对地址可以在`INSTALL_INTERFACE`中使用，会认为是相对与`prefix`的。
