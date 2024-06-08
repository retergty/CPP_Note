# CMAKE_LANG_FLAGS

参考文档

* [CMAKE_LANG_FLAGS](https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS.html)

指定当编译语言`LANG`时需要传递给编译器的参数字符串，包括编译器和链接器。

这一组变量是缓存变量，但是也可以在CMake中修改，CMake会在构建特定目标时,在当前目录作用域的**最后**传递当时的变量值。

如果未定义，则会在CMAke开始运行时从环境变量中获得初始值。

* `CMAKE_C_FLAGS`从`CFLAGS`环境变量获取初始值
* `CMAKE_CXX_FLAGS`从`CXXFLAGS`环境变量获取初始值
* `CMAKE_CUDA_FLAGS`从`CUDAFLAGS`环境变量获取初始值

这个变量直接传递给命令行，所以每个参数之间使用空格隔开，带有空格的参数应该使用`""`括起来。

这个参数会在变量`CMAKE_<LANG>_FLAGS_<CONFIG>`前传递给编译器。

来自变量的编译器参数,比如`CMAKE_LANG_FLAGS`等，会在来自属性的编译器参数，比如`COMPILE_FLAGS`，`COMPILE_OPTIONS`前被传递给编译器。
