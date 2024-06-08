# add_executable

添加一个类型为可执行文件的目标。

参考文档

* [add_executable](https://cmake.org/cmake/help/latest/command/add_executable.html)

## 命令格式

```CMake
add_executable(<name> [WIN32] [MACOSX_BUNDLE]
               [EXCLUDE_FROM_ALL]
               [source1] [source2 ...])
add_executable(<name> IMPORTED [GLOBAL])
add_executable(<name> ALIAS <target>)
```

## 详细描述

添加一个类型为可执行文件的目标，还有细分的三类，普通，导入，别名目标，使用三种格式，以下分别介绍。

这个命令支持生成期表达式。

### 普通可执行文件(Normal Executables)

```CMake
add_executable(<name> [WIN32] [MACOSX_BUNDLE]
               [EXCLUDE_FROM_ALL]
               [source1] [source2 ...])
```

给构建系统加入一个名字为`name`的可执行文件目标，这个目标使用源文件`source1 source2...`编译而来。

`name`为任意的字符串，是目标的逻辑名字。定义完毕后，它就拥有了全局可见性，在`CMakeFileLists.txt`中可以直接使用`name`引用这个目标.但实际生成的库文件名字取决于系统架构(`name.exe`或`name`).

`source1`为要编译的源文件，可以是相对路径，相对于当前源文件路径(`CMAKE_CURRENT_SOURCE_DIR`),之后也可以使用`target_sources()`再添加。

`WIN32`会设置目标的`WIN32_EXECUTABLE`属性。

`EXCLUDE_FROM_ALL`会把目标从`all`中排除。

默认来说，生成的可执行文件的位置就是`name`，翻译为相对路径，相对于当前二进制文件路径(`CMAKE_CURRENT_BINARY_DIR`),也可以通过`RUNTIME_OUTPUT_DIRECTORY`属性修改。

### 导入可执行文件(Imported Executables)

```CMake
add_executable(<name> IMPORTED [GLOBAL])
```

一个导入的可执行文件引用一个工程外的可执行文件，CMake不会去构建这个目标，因为它已经在工程外存在了。之后就可以方便地通过`name`引用这个可执行文件。

导入可执行文件会设置目标的`IMPORT`属性。并设置一系列以`IMPORT_`开头的目标属性。

`GLOBAL`声明导入文件全局可见，否则就只是当前及子作用域可见。

### 别名可执行文件(Alias Executables)

```CMake
add_executable(<name> ALIAS <target>)
```

给之前定义的目标`target`起一个别名`name`之后就可以使用这个名字引用目标。但是，**不能**通过这个名字修改目标属性。
