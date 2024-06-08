# CMAKE_SYSTEM_VERSION

参考文档

* [CMAKE_SYSTEM_VERSION](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_VERSION.html#variable:CMAKE_SYSTEM_VERSION)

CMake编译出可执行文件和库文件要运行在的操作系统的版本。

在非交叉编译时，不需要设置这个变量，会自动从`CMAKE_HOST_SYSTEM_VERSION`获得默认值。

在交叉编译时，需要设置这个变量，表明目标操作系统的版本。
