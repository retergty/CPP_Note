# CMAKE_CROSSCOMPILING

参考文档

* [CMAKE_CROSSCOMPILING](https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING.html)

CMake会设置这个变量值为真，表示目前在交叉编译。

要是手动设置了变量`CMAKE_SYSTEM_NAME`，那么CMake就会自动设置`CMAKE_CROSSCOMPILING`为真。
