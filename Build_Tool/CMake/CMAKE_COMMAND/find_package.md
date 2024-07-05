# find_package

寻找一个包(package)（通常是位于工程文件之外的），并读取与包有关的文件。

参考文档

* [find_package]([void](https://cmake.org/cmake/help/latest/command/find_package.html#id5))

## 命令格式

```CMake
find_package(<PackageName> [<version>] [REQUIRED] [COMPONENTS <components>...])
```

大部分的`find_package`都如上所示。

`<PackageName>`表示搜索的包名。

`<version>`表示指定包的必须符合的版本号。

`REQUIRED`表示如果没有找到包则报错，否则报警告。

`COMPONENTS`一些复杂的包还支持选择组件。

## 详细描述

一个包通常是早已构建并可以使用的文件。比如静态库，动态库，或者甚至是头文件库。一个包通常不位于工程文件之内，也可能并不是使用CMake构建的。`find_package`命令搜索常用的包安装路径，并使用两种搜索模式(search method)来尽可能地搜索到所需的包。

### 搜索模式

#### Config mode

这个模式需要包创建者提供一个配置文件(Config files),这个配置文件也是一个CMake文件，包含着一些目标的定义，头文件搜索目录等，`find_package`会直接读取这个文件。这是一个可靠的模式，大部分常见库都会提供这个配置文件。

#### Module mode

这个模式不需要包创建者在包内集成配置文件，而是由工程本身或者是CMake程序来提供一个模块文件(module file).`find_package`会读取这个文件。模块文件需要知道包会提供的内容以及把这些内容呈现给工程，通常是通过定义一些目标等。

### 配置文件

正如前文所说的，配置文件就是一个CMake格式的文件，配置文件通常会在类似于`lib/cmake/<PackageName>`的路径中，但是也可以配置其在不同的位置。

配置文件必须名为`<PackageName>Config.cmake`或`<LowercasePackageName>-config.cmake`.这个文件充当了一个CMake读取包信息的入口点。可选的文件`<PackageName>ConfigVersion.cmake`或`<LowercasePackageName>-config-version.cmake`指示这个包的版本号，用于与`find_package`中`<version>`做对比。

如果`<PackageName>Config.cmake`被找到且满足版本的要求，`find_package`认为包已被找到。

当然也可以使用更多的CMake文件表示包的信息，只要在对应的`<PackageName>Config.cmake`中`include()`就可以了。

可以使用`CMAKE_PREFIX_PATH`变量指定配置文件搜索的目录。这是一个列表，包含着要搜索的目录。还可以是环境变量`CMAKE_PREFIX_PATH`.

配置文件的搜索不是贪心的，只要它在一个目录中搜索到了对应的配置文件，它就结束搜索并读取了，为了让CMake搜索到新配制的配置文件，需要在`CMAKE_PREFIX_PATH`**前**添加对应目录。
