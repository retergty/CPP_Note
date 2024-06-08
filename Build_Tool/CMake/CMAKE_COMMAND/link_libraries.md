# link_libraries

添加要链接的库文件

参考文档

* [link_libraries](https://cmake.org/cmake/help/latest/command/link_libraries.html)

## 命令格式

```CMake
link_libraries([item1 [item2 [...]]]
               [[debug|optimized|general] <item>] ...)
```

## 详细描述

添加要链接的库文件，会影响在这之后创建的当前目录和子目录的目标的`LINK_LIBRARIES`目标属性。

`item`可以是之前通过`add_libraries()`命令创建出来的目标名，也可以是库文件名（通过别的命令指定搜索目录），也可以是库文件的路径，甚至单纯是链接参数。
