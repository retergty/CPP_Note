# project

设置工程的名字

参考文档

* [project](https://cmake.org/cmake/help/latest/command/project.html)

## 命令格式

```CMake
project(<PROJECT-NAME> [<language-name>...])
project(<PROJECT-NAME>
        [VERSION <major>[.<minor>[.<patch>[.<tweak>]]]]
        [DESCRIPTION <project-description-string>]
        [HOMEPAGE_URL <url-string>]
        [LANGUAGES <language-name>...])
```

## 详细描述

这个命令通常是在顶层的CMakeFile调用，设置工程的名字。在顶层的CMakeFile中调用时，会把`PROJECT-NAME`并存储到变量`PROJECT_NAME`和`CMAKE_PROJECT_NAME`中，在非顶层调用时就只会修改变量`PROJECT_NAME`。

这个命令还会设置一些如下的变量

* `PROJECT_SOURCE_DIR`和`<PROJECT-NAME>_SOURCE_DIR`命令`project()`调用时源文件的绝对路径
* `PROJECT_BINARY_DIR`和`<PROJECT-NAME>_BINARY_DIR`命令`project()`调用时二进制文件的绝对路径
* `PROJECT_IS_TOP_LEVEL`和`<PROJECT-NAME>_IS_TOP_LEVEL`当前的CMakeFile是不是顶层CMakeFile。

`LANGUAGES`参数声明使用的程序语言，支持的有`C`,`CXX`,`CSharp`,`CUDA`,`OBJC`,`OBJCXX`等。

不止如此，这个命令还会设置很多变量，比如`CMAKE_CXX_COMPILER`等。
