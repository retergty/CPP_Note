# add_custom_command

定义一个自定义命令。

参考文档

* [add_custom_command](https://cmake.org/cmake/help/latest/command/add_custom_command.html)

## 命令格式

```CMake
add_custom_command(OUTPUT output1 [output2 ...]
                   COMMAND command1 [ARGS] [args1...]
                   [COMMAND command2 [ARGS] [args2...] ...]
                   [MAIN_DEPENDENCY depend]
                   [DEPENDS [depends...]]
                   [BYPRODUCTS [files...]]
                   [IMPLICIT_DEPENDS <lang1> depend1
                                    [<lang2> depend2] ...]
                   [WORKING_DIRECTORY dir]
                   [COMMENT comment]
                   [DEPFILE depfile]
                   [JOB_POOL job_pool]
                   [JOB_SERVER_AWARE <bool>]
                   [VERBATIM] [APPEND] [USES_TERMINAL]
                   [COMMAND_EXPAND_LISTS]
                   [DEPENDS_EXPLICIT_ONLY])

add_custom_command(TARGET <target>
                   PRE_BUILD | PRE_LINK | POST_BUILD
                   COMMAND command1 [ARGS] [args1...]
                   [COMMAND command2 [ARGS] [args2...] ...]
                   [BYPRODUCTS [files...]]
                   [WORKING_DIRECTORY dir]
                   [COMMENT comment]
                   [VERBATIM]
                   [COMMAND_EXPAND_LISTS])
```

## 详细描述

这个命令定义一个自定义命令，分为两种格式，以下分别讲述。

### 生成文件

```CMake
add_custom_command(OUTPUT output1 [output2 ...]
                   COMMAND command1 [ARGS] [args1...]
                   [COMMAND command2 [ARGS] [args2...] ...]
                   [MAIN_DEPENDENCY depend]
                   [DEPENDS [depends...]]
                   [BYPRODUCTS [files...]]
                   [IMPLICIT_DEPENDS <lang1> depend1
                                    [<lang2> depend2] ...]
                   [WORKING_DIRECTORY dir]
                   [COMMENT comment]
                   [DEPFILE depfile]
                   [JOB_POOL job_pool]
                   [JOB_SERVER_AWARE <bool>]
                   [VERBATIM] [APPEND] [USES_TERMINAL]
                   [COMMAND_EXPAND_LISTS]
                   [DEPENDS_EXPLICIT_ONLY])
```

有时，工程可能创建一个或者多个文件，这些文件通过命令生成，不属于任何目标。当在同一个作用域下创建的目标将`OUTPUT`文件作为源文件时，CMake就会生成一个**规则**，这个规则使用这个命令在`build`阶段生成`OUTPUT`文件。

`OUTPUT`就是创建的文件。相对路径相对于当前二进制文件目录。不受`WORKING_DIRECTORY`的影响。

`DEPENDS`就是指定依赖，分以下几种情况

1. 如果指定的是**目标名**（使用`add_custom_target()`,`add_executable()`,`add_library()`创建的目标名），那么就会加入目标级别的依赖，保证指定的目标会在任何使用`output`的目标前运行。如果目标是可执行或者是库目标，还会加入文件级别的依赖，保证当指定的目标重新编译时，这个命令会运行。
2. 如果指定的是文件名，且这个文件名**已经加入到了某个目标中**或者是**已经设置了源文件属性**，那么就会加入文件级别的依赖，保证这个文件修改后，使用`output`的目标会重新构建。

注意，如果没有指定`DEPENDS`,那么命令就会在`OUTPUT`文件**不存在时运行**。如果指定了`DEPENDS`，那么命令就会在`OUTPUT`文件**不存在时**或者是`OUTPUT`文件**旧于**`DEPENDS`文件时运行。（可以用Makefile理解）。

### 添加构建步骤

```CMake
add_custom_command(TARGET <target>
                   PRE_BUILD | PRE_LINK | POST_BUILD
                   COMMAND command1 [ARGS] [args1...]
                   [COMMAND command2 [ARGS] [args2...] ...]
                   [BYPRODUCTS [files...]]
                   [WORKING_DIRECTORY dir]
                   [COMMENT comment]
                   [VERBATIM]
                   [COMMAND_EXPAND_LISTS])
```

给已经存在的`target`添加额外的构建步骤

`target`就是需要添加额外构架步骤的目标名

`PRE_BUILD | PRE_LINK | POST_BUILD`就是命令在构建`target`的何时运行，`PRE_BUILD`表示在构建`target`前运行，`PRE_LINK`表示在链接`target`前运行，`POST_BUILD`表示在构建完毕`target`后运行。

`BYPRODUCTS`声明命令预期产生的文件，这个文件**不是**该命令的依赖，会自动设置源文件属性`GENERATED`为`TRUE`.

`COMMAND`声明命令行命令，这些命令会在运行命令时按照声明的次序运行(`build`阶段)。命令的参数可以使用生成期表达式，通常使用的生成期表达式有`$<TARGET_FILE:tgt>`,`$<TARGET_LINKER_FILE:tgt>`.当使用生成期表达式后，`tgt`就自动变为`target`的依赖，从而保证在目标运行前，`tgt`首先被构建。

`COMMENT`声明在`build`阶段，运行命令时，要打印的消息。

`SOURCES`声明额外的源文件，这个方便IDE显示

`VERBATIM`声明所有的命令参数会被正确的防止转义，当然CMake处理时的转义会被保留，这个保护了命令参数不受运行环境的影响。

`USES_TERMINAL`声明命令可以直接访问终端。

`WORKING_DIRECTORY`声明命令运行的文件目录，相对路径会被翻译为相对于二进制文件目录(`CMAKE_CURRENT_BINARY_DIR`).

## 例子

```CMake
add_executable(MyExe main.cpp)

# Output file with relative path, generated in the
# build directory

add_custom_command(OUTPUT MyExe.md5
  COMMAND writeHash $<TARGET_FILE:MyExe>
) 

# Absolute path needed for DEPENDS, otherwise relative
# to source directory

add_custom_target(ComputeHash
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/MyExe.md5
)
```

这个例子声明了自定义命令，并且声明了自定义目标，自定义目标依赖于自定义命令，从而实现了运行自定义目标时，自定义命令的运行。
