# CMAKE_SYSTEM_NAME

参考文档

* [CMAKE_SYSTEM_NAME](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html#variable:CMAKE_SYSTEM_NAME)

CMake编译出的可执行文件与库文件要运行在的操作系统名。

在非交叉编译时，不需要设置这个变量，会自动从`CMAKE_HOST_SYSTEM_NAME`获得默认值。

设置这个变量后，必须再设置变量`CMAKE_SYSTEM_VERSION`.

通过手动设置这个变量名与`CMAKE_HOST_SYSTEM_NAME`不同，可以使能CMake交叉编译，CMake自动设置变量`CMAKE_CROSSCOMPILING`为`TRUE`.
