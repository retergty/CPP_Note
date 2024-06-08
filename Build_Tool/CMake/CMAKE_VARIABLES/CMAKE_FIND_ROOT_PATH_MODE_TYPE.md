# CMAKE_FIND_ROOT_PATH_MODE_TYPE

参考文档

* [CMAKE_FIND_ROOT_PATH_MODE_INCLUDE](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_INCLUDE.html)
* [CMAKE_FIND_ROOT_PATH_MODE_LIBRARY](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_LIBRARY.html)
* [CMAKE_FIND_ROOT_PATH_MODE_PACKAGE](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_PACKAGE.html)
* [CMAKE_FIND_ROOT_PATH_MODE_PROGRAM](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_PROGRAM.html)

这四个变量决定变量`CMAKE_FIND_ROOT_PATH`和`CMAKE_SYSROOT`是否被用于`find_*`一系列命令中。

设置这四个变量的值为`NEVER`,`BOTH`,`ONLY`.要是设置为`ONLY`只使用`CMAKE_FIND_ROOT_PATH`变量指定的搜索位置，也就是不使用`CMAKE_SYSROOT`指定的根目录搜索；要是设置为`NEVER`,就忽略`CMAKE_FIND_ROOT_PATH`变量，只使用`CMAKE_SYSROOT`变量指定的搜索位置；要是设置为`BOTH`两个变量都会使用。

* `CMAKE_FIND_ROOT_PATH_MODE_INCLUDE`

控制变量`CMAKE_FIND_ROOT_PATH`和`CMAKE_SYSROOT`是否被用于`find_file()`和`find_path()`中.

* `CMAKE_FIND_ROOT_PATH_MODE_LIBRARY`

控制变量`CMAKE_FIND_ROOT_PATH`和`CMAKE_SYSROOT`是否被用于`find_library()`.

* `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE`

控制变量`CMAKE_FIND_ROOT_PATH`和`CMAKE_SYSROOT`是否被用于`find_package()`.

* `CMAKE_FIND_ROOT_PATH_MODE_PROGRAM`

控制变量`CMAKE_FIND_ROOT_PATH`和`CMAKE_SYSROOT`是否被用于`find_program()`.
