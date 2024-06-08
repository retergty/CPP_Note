# CMake工具链(CMake Toolchains)

CMake使用工具链将源文件编译(compile),链接(link)为可执行文件，通常，CMake会自动选择合适的工具链，但是，对于交叉编译(cross-compile)任务，必须指定所用的工具链。

参考文档

* [CMake Toolchains](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)

## 语言

当运行命令`project()`时，CMake就会设置一系列的变量，与工具链有关的是

* `CMAKE_<LANG>_COMPILER`编译`LANG`时编译器的路径
* `CMAKE_<LANG>_COMPILER_ID`编译`LANG`时编译器的ID
* `CMAKE_<LANG>_COMPILER_VERSION` 编译器的版本
* `CMAKE_<LANG>_FLAGS`编译`LANG`传递给编译器的参数

## 编译检查

当第一次调用`project()`时，CMake就会进行编译器检查，它会编译一小段代码并运行，测试当前编译器的可用性。但有时，我们需要跳过编译检查，比如使用交叉编译编译器时，运行总是失败的。

通过设置变量`CMAKE_TRY_COMPILE_TARGET_TYPE`为`STATIC_LIBRARY`就可以避免使用链接器，从而对于交叉编译也可以通过检查。

## 工具链文件

要是默认的工具链不合适，推荐做法是在一个名叫工具链文件的文件中定义所需的工具链。工具链文件就是一个CMake脚本代码，通常只有`set()`命令。通过命令行

```CMake
cmake -DCMAKE_TOOLCHAIN_FILE=myToolchain.cmake path/to/src
```

就可以传递工具链文件，这个文件会在`path/to/src`里的`CMakeFileLists.txt`被读取前运行。可以使用相对路径，CMake会首先搜索当前二进制文件目录，之后搜索源文件目录。这个工具链文件可能会被Cmake读取**多次**。

通常工具链文件需要设置的变量如下

* `CMAKE_SYSTEM_NAME`
* `CMAKE_SYSTEM_PROCESSOR`
* `CMAKE_SYSTEM_VERSION`
* `CMAKE_<LANG>_COMPILER`
* `CMAKE_<LANG>_FLAGS_<CONFIG>_INIT`
* `CMAKE_<LANG>_COMPILER_VERSION`
* `CMAKE_SYSROOT`
* `CMAKE_STAGING_PREFIX`
* `CMAKE_FIND_ROOT_PATH_MODE_PROGRAM`
* `CMAKE_FIND_ROOT_PATH_MODE_LIBRARY`
* `CMAKE_FIND_ROOT_PATH_MODE_INCLUDE`
* `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE`

## 交叉编译

使用CMake进行交叉编译时，需要设置一系列的变量，包括`CMAKE_<LANG>_COMPILER`编译器选择，`CMAKE_SYSTEM_NAME`目标操作系统名，`CMAKE_SYSTEM_VERSION`目标操作系统版本号。

### Linux交叉编译

经典的Linux交叉编译需要设置的变量如下

```CMake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_SYSROOT /home/devel/rasp-pi-rootfs)
set(CMAKE_STAGING_PREFIX /home/devel/stage)

set(tools /home/devel/gcc-4.7-linaro-rpi-gnueabihf)
set(CMAKE_C_COMPILER ${tools}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

设置`CMAKE_SYSTEM_NAME`为目标系统名`Linux`.

设置`CMAKE_SYSTEM_PROCESSOR`为目标系统CPU架构`arm`.

设置`CMAKE_SYSROOT`为系统根目录.

设置`CMAKE_STAGING_PREFIX`声明一个位置，表示运行时安装位置，哪怕是在交叉编译的环境下。

最关键的两个变量`CMAKE_C_COMPILER`和`CMAKE_CXX_COMPILER`，设置了C和C++编译器的位置.

设置了变量`CMAKE_FIND_ROOT_PATH_MODE_PROGRAM`,`CMAKE_FIND_ROOT_PATH_MODE_LIBRARY`,`CMAKE_FIND_ROOT_PATH_MODE_INCLUDE`,`CMAKE_FIND_ROOT_PATH_MODE_PACKAGE`.这四个变量影响`find_*`命令的运行，由于查找到的程序通常是运行在主机上的，所以设置`CMAKE_FIND_ROOT_PATH_MODE_PROGRAM`为`NEVER`也就是不考虑`CMAKE_FIND_ROOT_PATH`的值，其它的一般指的都是目标平台的文件，所以使用`ONLY`。注意！这四个变量**不影响**编译器搜索库文件，头文件的路径(`CMAKE_SYSROOT`影响)，只是影响`find_*`系列命令的运行。
