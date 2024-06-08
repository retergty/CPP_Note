# include_directories

添加编译时的搜索目录。

参考文档

* [include_directories](https://cmake.org/cmake/help/latest/command/include_directories.html)

## 命令格式

```CMake
include_directories([AFTER|BEFORE] [SYSTEM] dir1 [dir2 ...])
```

## 详细描述

添加编译器编译时的搜索目录，相对路径会被翻译为相对于当前的源文件目录。

`dir1`会被添加到当前目录的`INCLUDE_DIRECTORIES`属性中，同时还会添加到在当前目录已创建的目标的`INCLUDE_DIRECTORIES`属性中。**注意**，不会添加到当前目录已创建的子目录目标的`INCLUDE_DIRECTORIES`属性。由于创建的目标会继承当前目录的`INCLUDE_DIRECTORIES`属性，所以之后的目标都会受到`include_directories`的影响。

`[AFTER|BEFORE]`指定添加属性时是添加到前面(BEFORE)或者是添加到后面(AFTER),默认是添加到后面。

`SYSTEM`告诉某些平台的编译器这个目录是系统的头文件的搜索目录(system include directories)，这个参数的实际效果需要参考编译器文档。

最后，这个命令支持正则表达式。
