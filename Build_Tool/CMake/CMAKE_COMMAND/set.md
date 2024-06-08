# set

设置通常变量，设置缓存变量，设置环境变量

参考文档

* [set](https://cmake.org/cmake/help/latest/command/set.html)

## 命令格式

```CMake
set(<variable> <value>... [PARENT_SCOPE])
set(<variable> <value>... CACHE <type> <docstring> [FORCE])
set(ENV{<variable>} [<value>])
```

## 详细描述

设置通常变量，设置缓存变量，设置环境变量

### 设置通常变量

```CMake
set(<variable> <value>... [PARENT_SCOPE])
```

如果多个`value`被给出，且没有双引号包括他们，那么`CMAKE`就会把他们连在一起，用`;`分隔，这种类型的变量就叫做**列表(LIST)**

```CMake
set(myVar a b c)   # myVar = "a;b;c"
set(myVar a;b;c)   # myVar = "a;b;c"
set(myVar "a b c") # myVar = "a b c"
set(myVar a b;c)   # myVar = "a;b;c"
set(myVar a "b c") # myVar = "a;b c"
```

`PARENT_SCOPE`表示在父作用域定义这个变量，这个并不意味着会修改子作用域的变量，它只会修改父作用域的变量，子作用域的同名变量不受影响。

### 设置缓存变量

```CMake
set(<variable> <value>... CACHE <type> <docstring> [FORCE])
```

使用关键字`CACHE`指明这个变量是缓存变量

缓存变量必须设置`type`和`docstring`，这个不会影响CMake处理变量的流程，但会改变`CMake GUI`显示这个变量的方法。

`type`必须是如下几个值

* `BOOL`

缓存变量是一个布尔值，`GUI`使用一个方框勾选的方法显示这个缓存变量。

* `FILEPATH`

缓存变量是一个文件路径，`GUI`使用文件目录表来显示这个缓存变量。

* `PATH`

缓存变量是一个文件夹，`GUI`使用文件目录表来显示这个缓存变量。

* `STRING`

缓存变量是一个字符串，`GUI`使用文本框显示这个缓存变量。

* `INTERNAL`

缓存变量是一个内部变量，不希望显示给用户，`GUI`不显示这个缓存变量。这种类型的缓存变量默认是`FORCE`的。

`docstring`是任意的字符串，用来在`GUI`中描述变量。

默认情况下，如果缓存变量已经存在，则`set()`命令**不会运行**，哪怕是该命令设置的缓存变量值和已经存在的缓存变量值不同。

`FORCE`可以使得`set()`命令**总是运行**，从而**总是**会覆盖原缓存变量的值。注意，`FORCE`关键字与缓存变量设计的初衷不匹配，缓存变量的设计是希望用户可以通过命令行或者是`GUI`工具方便的修改，而不需要修改CMakeFileLists.txt.

### 设置环境变量

```CMake
set(ENV{<variable>} [<value>])
```

CMake允许获取和设置系统当前的环境变量，通过特殊的格式`$ENV{varName}`来获取或是设置系统当前的环境变量。
