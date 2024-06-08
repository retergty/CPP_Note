# CMAKE_LANG_FLAGS_CONFIG

参考文档

* [CMAKE_LANG_FLAGS_CONFIG](https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_FLAGS_CONFIG.html#variable:CMAKE_%3CLANG%3E_FLAGS_%3CCONFIG%3E)

指定当编译语言`LANG`以及构建配置选择为`CONFIG`时需要传递给编译器的参数字符串，包括编译器和链接器。

`CONFIG`忽略大小写。

这一组变量是缓存变量，但是也可以在CMake中修改，CMake会在构建特定目标时,在当前目录作用域的**最后**传递当时的变量值。

对于特定的构建类型与编译器，这组变量会有默认值

对于`CMAKE_CXX_FLAGS_CONFIG`，默认值如下

* `CMAKE_CXX_FLAGS_DEBUG`有默认值`-g -O0`
* `CMAKE_CXX_FLAGS_RELEASE`有默认值`-O3 -DNDEBUG`
* `CMAKE_CXX_FLAGS_RELWITHDEBINFO`有默认值`-O2 -g -DNDEBUG`
* `CMAKE_CXX_FLAGS_MINSIZEREL`有默认值`-Os -DNDEBUG`
