# include

从文件或者模块中读取CMake代码，并粘贴到调用处。

参考文档

* [include]((https://cmake.org/cmake/help/latest/command/include.html))

## 命令格式

```CMake
include(<file|module> [OPTIONAL] [RESULT_VARIABLE <var>]
                      [NO_POLICY_SCOPE])
```

## 详细描述

从文件或者模块中粘贴CMake代码到调用处，并立即读取，**不创造**一个新的作用域。

`<file|module>`可以是文件绝对路径或者是相对路径，相对于当前源文件目录。

`OPTIONAL`表示要是文件不存在，不会报错，而是直接忽略。

`RESULT_VARIABLE <var>`表示结果存储在`var`中，要是文件找到，则是文件的绝对路径，要是文件没有找到，则是`NOTFOUND`.
