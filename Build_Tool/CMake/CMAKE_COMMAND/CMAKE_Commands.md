# CMake 常用命令

本文件夹汇总了CMake常用的命令。

CMake命令大全

* CMake 官方文档[CMake Commands](https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html)

## 标准命令索引

标准命令指的是不需要使用`include()`包含CMake模块就可以使用的命令。

### 用于整个工程的命令

#### [`cmake_minimum_required()`](cmake_minimum_required.md)

#### [`project()`](project.md)

#### [`include()`](include.md)

#### [`add_subdirectory()`](add_subdirectory.md)

#### [`set()`](set.md)

#### [`option()`](option.md)

#### [`mark_as_advanced`](mark_as_advanced.md)

#### [`set_property()`](set_property.md)

#### [`install()`](install.md)

### 用于CMake语法的命令

#### [`cmake_parse_arguments()`](cmake_parse_arguments.md)

#### [`foreach()`](foreach.md)

### 用于处理文本的命令

#### [`list()`](list.md)

#### [`string()`](string.md)

### 用于目录的命令

#### [`include_directories()`](include_directories.md)

#### [`add_definitions()`](add_definitions.md)

#### [`remove_definitions()`](remove_definitions.md)

#### [`add_compile_definitions()`](add_compile_definitions.md)

#### [`add_compile_options()`](add_compile_options.md)

#### [`link_libraries()`](link_libraries.md)

#### [`link_directories()`](link_directories.md)

#### [`add_link_options()`](add_link_options.md)

### 用于文件的命令

#### [`configure_file()`](configure_file.md)

#### [`file()`](file.md)

### 用于目标的命令

#### [`add_executable()`](add_executable.md)

#### [`add_library()`](add_library.md)

#### [`target_link_libraries()`](target_link_libraries.md)

#### [`target_link_directories()`](target_link_directories.md)

#### [`target_link_options()`](target_link_options.md)

#### [`target_include_directories()`](target_include_directories.md)

#### [`target_compile_options()`](target_compile_options.md)

#### [`target_compile_definitions()`](target_compile_definitions.md)

#### [`target_compile_features()`](target_compile_features.md)

#### [`target_sources()`](target_sources.md)

#### [`add_custom_target()`](add_custom_target.md)

#### [`add_custom_command()`](add_custom_command.md)

#### [`add_dependencies()`](add_dependencies.md)

### 用于测试的命令

#### [`enable_testing()`](enable_testing.md)

#### [`add_test()`](add_test.md)

## 特殊命令索引

标准命令指的是需要使用`include()`包含CMake模块才能使用的命令。

### [`ExternalProject`](Module/ExternalProject.md)

### [`FetchContent`](Module/FetchContent.md)
