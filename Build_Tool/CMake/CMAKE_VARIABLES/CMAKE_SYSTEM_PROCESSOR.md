# CMAKE_SYSTEM_PROCESSOR

参考文档

* [CMAKE_SYSTEM_PROCESSOR](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_PROCESSOR.html#variable:CMAKE_SYSTEM_PROCESSOR)

CMake编译出的可执行文件与库文件要运行在的CPU架构。

在非交叉编译时，不需要设置这个变量，会自动从`CMAKE_HOST_SYSTEM_PROCESSOR`获得默认值。

在交叉编译时，设置目标CPU架构，比如`arm`.
