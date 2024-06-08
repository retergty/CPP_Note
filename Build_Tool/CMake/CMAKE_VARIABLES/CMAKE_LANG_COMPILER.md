# CMAKE_LANG_COMPILER

参考文档

* [CMAKE_LANG_COMPILER](https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER.html#variable:CMAKE_%3CLANG%3E_COMPILER)

用于编译语言`LANG`的编译器的路径，可以是相对路径，CMake会根据`PATH`环境变量搜索。也可以包含传递给编译器的参数

```CMake
#set within user supplied toolchain file
set(CMAKE_C_COMPILER /full/path/to/qcc --arg1 --arg2)
```
