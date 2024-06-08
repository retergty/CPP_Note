# configure_file

复制指定的文件，并修改其内容

参考文档

* [configure_file](https://cmake.org/cmake/help/latest/command/configure_file.html#command:configure_file)

## 命令格式

```CMake
configure_file(<input> <output>
               [NO_SOURCE_PERMISSIONS | USE_SOURCE_PERMISSIONS |
                FILE_PERMISSIONS <permissions>...]
               [COPYONLY] [ESCAPE_QUOTES] [@ONLY]
               [NEWLINE_STYLE [UNIX|DOS|WIN32|LF|CRLF] ])
```

## 详细描述

复制指定的文件，并修改其内容，使用CMake变量替换指定的内容。

CMake会替换输入文件中形如`@var@`,`${VAR}`,`$CACHE{VAR}`,`$ENV{VAR}`的文本为CMake当前对应变量（或缓存变量，环境变量）的值。要是该变量未定义，则替换为空字符串。

除此以外，CMake还会对输入进行如下操作

* 对于输入文本

```CMake
#cmakedefine VAR ...
```

会被替换为

```CMake
#define VAR ...
```

或者是

```CMake
/* #undef VAR */
```

取决于当前作用域`VAR`是否被定义。

* 对于输入文本

```CMake
#cmakedefine01 VAR
```

会被替换为

```CMake
#define VAR 0
```

或者是

```CMake
#define VAR 1
```

取决于当前作用域`VAR`是否被定义。

`input`为要复制的文件的路径，可以是相对路径，相对于当前的源文件路径`CMAKE_CURRENT_SOURCE_DIR`.

`output`为复制并修改文件存储的文件名，可以是相对路径，相对于当前的二进制文件路径`CMAKE_CURRENT_BINARY_DIR`.如果没有对应的目录则会自动创建。

`[NO_SOURCE_PERMISSIONS | USE_SOURCE_PERMISSIONS |FILE_PERMISSIONS <permissions>...]`可选的选项声明复制的文件的权限.

`COPYONLY`表示不修改内容

`ESCAPE_QUOTES`使用反斜杠转义任何替换引号（C 样式）.

`@ONLY`替换形如`@var@`的变量，而不替换`${var}`

`NEWLINE_STYLE`换行符的类型，`\n`还是`\r\n`.

## 例子

考虑文件`foo.h.in`有如下内容

```CMake
#cmakedefine FOO_ENABLE
#cmakedefine FOO_STRING "@FOO_STRING@"
```

使用CMake代码如下

```CMake
option(FOO_ENABLE "Enable Foo" ON)
if(FOO_ENABLE)
  set(FOO_STRING "foo")
endif()
configure_file(foo.h.in foo.h @ONLY)
```

生成的文件`foo.h`有如下内容,若`FOO_ENBALE`为`ON`.

```C
#define FOO_ENABLE
#define FOO_STRING "foo"
```

否则

```C
/* #undef FOO_ENABLE */
/* #undef FOO_STRING */
```

由于生成的文件是在构建目录里的，所以需要使用`target_include_directories()`包含这个目录。
