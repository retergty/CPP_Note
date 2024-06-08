# FetchContent

不同于`ExternalProject`模块在`build`阶段下载内容，`FetchContent`模块可以在`configure`阶段下载内容，从而在对应的命令被CMake读取后，立刻使得它指向的内容可用。那么可以被用于别的命令。

参考文档

* [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html#fetchcontent)

## FetchContent_Declare

```CMake
FetchContent_Declare(
  <name>
  <contentOptions>...
  [EXCLUDE_FROM_ALL]
  [SYSTEM]
  [OVERRIDE_FIND_PACKAGE |
   FIND_PACKAGE_ARGS args...]
)
```

### FetchContent_Declare命令描述

`FetchContent_Declare`不会实际下载内容，而是声明了如何下载给定的内容。如果之前已经指定了`name`的内容，那么之后的`FetchContent_Declare`的`name`就会忽略。

`<contentOptions>`可以是`ExternalProject_Add()`命令理解的任何下载、更新或补丁选项。

大部分时候，`FetchContent_Declare`只是指定了下载`url`或者是`git`仓库。

```CMake
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        703bd9caab50b139428cea1aaff9974ebee5742e # release-1.10.0
)

FetchContent_Declare(
  myCompanyIcons
  URL      https://intranet.mycompany.com/assets/iconset_1.12.tar.gz
  URL_HASH MD5=5588a7b18261c20068beabfb4f530b87
)
```

`EXCLUDE_FROM_ALL`在使用命令`FetchContent_MakeAvailable()`加入的子目录里的目标不会在`ALL`里。

## FetchContent_MakeAvailable

```CMake
FetchContent_MakeAvailable(<name1> [<name2>...])
```

这个命令保证每个`name`在这个命令调用完毕后都是可用状态。`name`必须之前通过`FetchContent_Declare`方法声明。

如果填充的内容的顶层目录里含有`CMakeLists.txt`，那么这个命令就会调用`add_subdirectory()`命令，把这个目录加入到主构建系统中。

## FetchContent_Populate

```CMake
FetchContent_Populate(<name>)
```

填充`name`指定的内容，`name`必须之前通过`FetchContent_Declare`方法声明。通常我们使用`FetchContent_MakeAvailable`而不是`FetchContent_Populate`.因为`FetchContent_MakeAvailable`内部也会调用`FetchContent_Populate`.但是如果我们想自主构建填充的内容的话，使用`FetchContent_MakeAvailable`可能就不是一个好办法。

`FetchContent_Populate`会在当前作用域设置三个变量。

`<lowercaseName>_POPULATED`调用完毕`FetchContent_Populate`后，这个变量为真

`<lowercaseName>_SOURCE_DIR`填充的内容放在的文件目录。

`<lowercaseName>_BINARY_DIR`用于存放生成的二进制文件的文件目录
