# add_custom_target

加入一个没有输出文件目标，这个目标总是会被构建。注意，这不意味着每次CMake都会选择构建这个目标，而是当CMake**需要构建这个目标时，这个目标总是会被构建**。

参考文档

* [add_custom_target](https://cmake.org/cmake/help/latest/command/add_custom_target.html)

## 命令格式

```CMake
add_custom_target(Name [ALL] [command1 [args1...]]
                  [COMMAND command2 [args2...] ...]
                  [DEPENDS depend depend depend ... ]
                  [BYPRODUCTS [files...]]
                  [WORKING_DIRECTORY dir]
                  [COMMENT comment]
                  [JOB_POOL job_pool]
                  [JOB_SERVER_AWARE <bool>]
                  [VERBATIM] [USES_TERMINAL]
                  [COMMAND_EXPAND_LISTS]
                  [SOURCES src1 [src2...]])
```

## 详细描述

向构建系统加入一个名为`Name`的目标，这个目标执行`command`。这个目标不生成文件，所以总是会被认为是过时的(out of date).

`ALL`表示这个目标加入`all`默认构建目标中

`BYPRODUCTS`声明目标预期产生的文件，这个文件**不是**该目标的依赖，会自动设置源文件属性`GENERATED`为`TRUE`.当这个预期产生的文件作为了当前作用域下别的目标的源文件，那么这个目标就会依赖于自定义目标。

`DEPENDS`声明自定义目标的依赖，CMake会保证依赖先于该目标被更新。

`COMMAND`声明命令行命令，这些命令会在构建这个目标时按照声明的次序运行(`build`阶段)。命令的参数可以使用生成期表达式，通常使用的生成期表达式有`$<TARGET_FILE:tgt>`,`$<TARGET_LINKER_FILE:tgt>`.当使用生成期表达式后，`tgt`就自动变为`name`自定义目标的依赖，从而保证在自定义目标运行前，`tgt`首先被构建。

`COMMENT`声明在`build`阶段，构建自定义目标时，要打印的消息。

`SOURCES`声明额外的源文件，这个方便IDE显示

`VERBATIM`声明所有的命令参数会被正确的防止转义，当然CMake处理时的转义会被保留，这个保护了命令参数不受运行环境的影响。

`COMMAND_EXPAND_LISTS`声明如果命令的参数`arg`是列表的话（包括由生成期表达式生成的列表），会正确的传递给命令行。

`USES_TERMINAL`声明命令可以直接访问终端。

`WORKING_DIRECTORY`声明命令运行的文件目录，相对路径会被翻译为相对于二进制文件目录(`CMAKE_CURRENT_BINARY_DIR`).

## 例子

```CMake
# Platform independent equivalent
add_custom_target(MyCleanup
  COMMAND "${CMAKE_COMMAND}" -E rm -R "${discardDir}"
)
```

创建了一个`MyCleanup`的自定义目标，在指定构建这个目标时，会清除`${dircardDir}`变量指定的目录以及该目录下的所有文件。
