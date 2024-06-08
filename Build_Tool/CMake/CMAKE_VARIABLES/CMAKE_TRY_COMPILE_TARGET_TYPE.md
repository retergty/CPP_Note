# CMAKE_TRY_COMPILE_TARGET_TYPE

参考文档

* [CMAKE_TRY_COMPILE_TARGET_TYPE](https://cmake.org/cmake/help/latest/variable/CMAKE_TRY_COMPILE_TARGET_TYPE.html)

命令`try_compile()`编译出的目标类型，通常在第一次运行`project()`时，CMake就会编译一段测试代码并运行，检查编译器可用性。对于交叉编译，我们可以把这个变量的值设置为`STATIC_LIBRARY`。默认值是`EXECUTABLE`.
