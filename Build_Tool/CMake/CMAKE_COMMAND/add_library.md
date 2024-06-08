# add_library

添加一个类型为库文件的目标

参考文档

* [add_library](https://cmake.org/cmake/help/latest/command/add_library.html)

## 命令格式

```CMake
add_library(<name> [STATIC | SHARED | MODULE]
            [EXCLUDE_FROM_ALL]
            [<source>...])
add_library(<name> INTERFACE)
add_library(<name> <type> IMPORTED [GLOBAL])
add_library(<name> ALIAS <target>)
```

这个命令支持生成期表达式。

## 详细描述

添加一个类型为库文件的目标，还有细分的类型

### 普通库文件(Normal Libraries)

```CMake
add_library(<name> [STATIC | SHARED | MODULE]
            [EXCLUDE_FROM_ALL]
            [<source>...])
```

给构建系统加入一个名字为`name`的库文件目标，这个目标使用源文件`source1 source2...`编译而来。

`name`为任意的字符串，是目标的逻辑名字。定义完毕后，它就拥有了全局可见性，在`CMakeFileLists.txt`中可以直接使用`name`引用这个目标.但实际生成的库文件名字取决于系统架构(比如`lib<name>.a`或`<name>.lib`).

`[STATIC | SHARED | MODULE]`表示要编译出的库文件的类型，静态库，共享库，模块。静态库只是单纯的`.o`文件的集合，用于后续链接成可执行文件。共享库是在运行时加载并链接的。模块是一个插件。对于共享库和模块，目标属性`POSITION_INDEPENDENT_CODE`会自动设置为`ON`.

`source1`为要编译的源文件，可以是相对路径，相对于当前源文件路径(`CMAKE_CURRENT_SOURCE_DIR`),之后也可以使用`target_sources()`再添加。

`EXCLUDE_FROM_ALL`会把目标从`all`中排除。

默认来说，生成的可执行文件的位置就是`name`，翻译为相对路径，相对于当前二进制文件路径(`CMAKE_CURRENT_BINARY_DIR`),也可以通过`ARCHIVE_OUTPUT_DIRECTORY`或`LIBRARY_OUTPUT_DIRECTORY`属性修改。

### 接口库文件(Interface Libraries)

```CMake
add_library(<name> INTERFACE)
```

接口库文件通常只是链接给别的目标，传递信息，比如头文件搜索目录，编译器预处理器定义等等，通常使用方法是设置`INTERFACE_`开头的属性，并使用`target_link_libraries()`.

接口库文件也可以有源文件，但是CMake不会编译这些源文件为接口库文件。可能会在后续自定义目标使用。

### 导入库文件(Imported Libraries)

```CMake
add_library(<name> <type> IMPORTED [GLOBAL])
```

一个导入库文件引用一个工程外的库文件，CMake不会去构建这个目标，因为它已经在工程外存在了。之后就可以方便地通过`name`引用这个库文件。

导入库文件会设置目标的`IMPORT`属性。并设置一系列以`IMPORT_`开头的目标属性。

`GLOBAL`声明导入文件全局可见，否则就只是当前及子作用域可见。

`type`可以是以下几个值

* `STATIC`, `SHARED`, `MODULE`, `UNKNOWN`
* `OBJECT`
* `INTERFACE`

### 别名库文件(Alias Libraries)

```CMake
add_library(<name> ALIAS <target>)
```

给之前定义的目标`target`起一个别名`name`之后就可以使用这个名字引用目标。但是，**不能**通过这个名字修改目标属性。
