# foreach

循环并赋值

参考文档

* [foreach](https://cmake.org/cmake/help/latest/command/foreach.html)

## 命令格式

```CMake
foreach(loopVar IN [LISTS listVar1 ...] [ITEMS item1 ...])
    # ...
endforeach()
```

## 详细描述

`LISTS`关键字指明所跟的参数是一个列表变量需要被展开，`ITEMS`关键字指明所跟的参数是字符串不需要被展开。

每次循环，`loopvar`会被赋值为`IN`关键字后面指明的列表。

```CMake
set(list1 A B)
set(list2)
set(foo WillNotBeShown)
foreach(loopVar IN LISTS list1 list2 ITEMS foo bar)
    message("Iteration for: ${loopVar}")
endforeach()
```

输出为

```Text
Iteration for: A
Iteration for: B
Iteration for: foo
Iteration for: bar
```
