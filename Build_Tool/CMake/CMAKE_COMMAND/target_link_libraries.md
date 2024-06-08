# target_link_libraries

表明特定目标链接的库文件或者是传递给编译器的参数

参考文档

* [target_link_libraries](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)

## 命令格式

```CMake
target_link_libraries(<target> ... <item>... ...)
```

## 详细描述

`target`就是之前通过`add_executable()`或`add_library()`命令创建的目标名。

`item`可以是以下几个类型

* **库文件目标名**，是之前通过`add_library()`命令创建或者是`IMPORTED`的目标名,CMake会自动处理链接这一类库的文件路径。
* **库文件的完整路径**
* **库文件名**，要求链接器自己去搜索库文件，搜索目录可以通过`target_link_directories()`指定。
* **单纯的链接参数**，对于`item`开始于`-`且不是`-l`的，CMake认为是链接参数，直接传递给链接器。
* **生成期表达式**

## 声明使用要求

```CMake
target_link_libraries(<target>
                      <PRIVATE|PUBLIC|INTERFACE> <item>...
                     [<PRIVATE|PUBLIC|INTERFACE> <item>...]...)
```

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`LINK_LIBRARIES`、`INTERFACE_LINK_LIBRARIES`属性添加新的表项.若是不指定参数，则默认是`PUBLIC`.

同时，`item`里所有的以`INTERFACE_`的属性都会传递给`target`中的同名非`INTERFACE_`的属性，从而实现了使用要求。
