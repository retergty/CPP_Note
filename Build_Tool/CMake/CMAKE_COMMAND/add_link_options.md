# add_link_options

添加链接时传递给链接器的参数，只对可执行文件，共享库，模块有效果，对静态库没有效果。

参考文档

* [add_link_options](https://cmake.org/cmake/help/latest/command/add_link_options.html)

## 命令格式

```CMake
add_link_options(<option> ...)
```

## 详细描述

添加链接时传递给链接器的参数，添加`<option>`到当前目录的`LINK_OPTIONS`属性。

最后传递给编译器的参数由目标自己的`LINK_OPTIONS`属性决定，（包括它从别的库中继承的使用要求）。之后，CMake会自动去除重复项。

这个命令支持生成期表达式。
