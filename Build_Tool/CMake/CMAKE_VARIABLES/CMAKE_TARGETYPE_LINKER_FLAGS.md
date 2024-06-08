# CMAKE_TARGETYPE_LINKER_FLAGS

参考文档

* [CMAKE_SHARED_LINKER_FLAGS](https://cmake.org/cmake/help/latest/variable/CMAKE_SHARED_LINKER_FLAGS.html)
* [CMAKE_EXE_LINKER_FLAGS](https://cmake.org/cmake/help/latest/variable/CMAKE_EXE_LINKER_FLAGS.html)
* [CMAKE_STATIC_LINKER_FLAGS](https://cmake.org/cmake/help/latest/variable/CMAKE_STATIC_LINKER_FLAGS.html)
* [CMAKE_MODULE_LINKER_FLAGS](https://cmake.org/cmake/help/latest/variable/CMAKE_MODULE_LINKER_FLAGS.html)

当在构建特定目标类型时，传递给链接器的变量的字符串。

这一组变量是缓存变量，但是也可以在CMake中修改，CMake会在构建特定目标时,在当前目录作用域的**最后**传递当时的变量值。
