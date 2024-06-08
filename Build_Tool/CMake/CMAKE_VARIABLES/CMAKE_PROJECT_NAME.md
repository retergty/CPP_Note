# CMAKE_PROJECT_NAME

参考文档

* [CMAKE_PROJECT_NAME](https://cmake.org/cmake/help/latest/variable/CMAKE_PROJECT_NAME.html#variable:CMAKE_PROJECT_NAME)

整个工程的工程名，在顶层CMakeFile中**最近**一次调用`project()`所设置的值。

```CMake
cmake_minimum_required(VERSION 3.0)
project(First)
project(Second)
add_subdirectory(sub)
project(Third)
```

在子目录`sub`中，`CMAKE_PROJECT_NAME`就是`Second`.
