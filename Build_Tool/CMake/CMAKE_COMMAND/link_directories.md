# link_directories

添加库文件查找目录

参考文档

* [link_directories](https://cmake.org/cmake/help/latest/command/link_directories.html)

## 命令格式

```CMake
link_directories([AFTER|BEFORE] directory1 [directory2 ...])
```

## 详细描述

添加库文件的查找目录，给目录属性`LINK_DIRECTORIES`添加对应的表项.

`directory`可以是绝对路径，也可以是相对路径，相对路径被翻译为相对于当前的源文件目录。

这个命令支持生成期表达式。
