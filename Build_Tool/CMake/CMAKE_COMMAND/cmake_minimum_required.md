# cmake_minimum_required

声明所需的最低的CMake版本号

参考文档

* [cmake_minimum_required](https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html)

## 命令格式

```CMake
cmake_minimum_required(VERSION <min>[...<policy_max>] [FATAL_ERROR])
```

## 详细描述

声明所需的最低的CMake版本号，要是运行的CMake版本低于所声明的，CMake会停止并报错。

`VERSION`声明了CMake的最低版本号比如`3.12`等。
