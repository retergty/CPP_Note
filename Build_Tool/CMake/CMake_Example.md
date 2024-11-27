# CMake Example

本文讲解一些使用`CMake`的例子，都是一些难以解决的实例。

## 在源文件编译为`.o`文件之前运行自定义命令

在源文件`.cpp`编译为`.o`文件**前**，运行自定义命令。这个自定义命令需要满足以下条件

1. **当且仅当**`.cpp`文件**确实**需要重新编译为`.o`文件时才运行这个命令，也就是说，只有对应的`.cpp`文件修改后，运行这个命令.
2. 这个命令必须在对应的`.cpp`文件编译为`.o`文件**前**运行.

基于以上的要求，我们以CPP覆盖测试(coverage test)为例.

根据`gcov`的文档，当指定编译选项`--coverage`时，就会在编译`.cpp`文件为`.o`文件的同时，生成一个对应的`.gcno`文件。但是，如果在生成`.gcno`文件时，当前目录已经存在了同名的`.gcno`文件，那么`g++`就不会生成`.gcno`文件。这样，错误的`.gcno`文件就会带来错误的覆盖测试结果。不幸的是，我们使用CMake构建器时，CMake可以保证`.cpp`文件修改时，重新编译为`.o`，但是却不保证在此之前删除`.gcno`文件。此外，如果我们只是在CMake重新构建目标前简单地删除所有的`.gcno`文件，那么那些未被修改的`.cpp`文件却不会重新编译为`.o`，就会带来`.gcno`文件缺失。

解决方法如下

首先，我们知道`.o`文件的存放位置是在`${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${test_target}.dir`里的，所以生成的`.gcno`文件也是在这个文件夹里。我们为每个`.cpp`文件创建一个自定义命令，这个自定义命令产生一个空白文件输出作为标志。比如对于`main`目标的`hello.cpp`，这个自定义命令产生`./CMakeFiles/main.dir/clear_main_hello.cpp_coverage.h`,在这个自定义命令里，我们删除`hello.cpp.gcno`文件，同时`touch clear_main_hello.cpp_coverage.h`保证这个文件修改时间新于自定义命令的依赖`hello.cpp`。

之后，我们把这个空白文件`touch clear_main_hello.cpp_coverage.h`作为目标`main`的源文件，`.h`的后缀名告诉CMake不要把这个文件编译为`.o`文件(也就是自动设置源文件属性`HEADER_FILE_ONLY`)。这样就保证了这个命令的运行。

当CMake考虑重新构建`main`目标时，它就会先考虑它的源文件是否需要重新构建，此时就会考虑`clear_main_hello.cpp_coverage.h`是否需要重新构建，

```CMake
add_executable(main main.cpp hello.cpp)

list(APPEND test_targets main)

foreach(test_target IN LISTS test_targets)
  get_property(target_srcs TARGET ${test_target} PROPERTY SOURCES)
  foreach(target_src IN LISTS target_srcs)
    add_custom_command(OUTPUT ./CMakeFiles/${test_target}.dir/clear_${test_target}_${target_src}_coverage.h
    DEPENDS ${target_src}
    COMMAND ${CMAKE_COMMAND} -E echo "clearing ${target_src}.gcno"
    COMMAND ${CMAKE_COMMAND} -E rm -f ./${target_src}.gcno
    COMMAND ${CMAKE_COMMAND} -E touch clear_${test_target}_${target_src}_coverage.h
    WORKING_DIRECTORY ./CMakeFiles/${test_target}.dir
    )
    target_sources(${test_target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${test_target}.dir/clear_${test_target}_${target_src}_coverage.h")
  endforeach()
endforeach()
```

## 在重新构建时删除特定文件

在重新构建时，删除特定文件。需要满足以下条件

1. 每次运行`cmake --build .`时都会删除特定的文件。
2. 删除时允许文件不存在

基于以上的要求，我们以CPP覆盖测试(coverage test)为例.

根据`gcov`的文档，如果我们运行了生成的可执行文件，那么就会为所有`.o`文件创建运行时信息`.gcda`文件。当我们修改了源文件后，`.gcda`文件应该删除。此外，如果我们显式指定`cmake --build .`时，也必须删除`.gcda`文件，否则测试信息错误。

解决方法如下

我们为测试目标添加一个自定义目标，这个自定义目标没有输出，所以永远都是过时的(out of date),所以我们就可以在这个自定义目标中，删除所有的`.gcda`文件。

```CMake
add_executable(main main.cpp Hello.cpp)

list(APPEND test_targets main)

foreach(test_target IN LISTS test_targets)
  get_property(target_srcs TARGET ${test_target} PROPERTY SOURCES)
  foreach(target_src IN LISTS target_srcs)
    list(APPEND ${test_target}_gcda ./${target_src}.gcda)
  endforeach()

  add_custom_target(clear_${test_target}_gcda 
  COMMAND ${CMAKE_COMMAND} -E echo "clearing ${${test_target}_gcda}"
  COMMAND ${CMAKE_COMMAND} -E rm -f "${${test_target}_gcda}" 
  WORKING_DIRECTORY ./CMakeFiles/${test_target}.dir
  COMMAND_EXPAND_LISTS)
  
  add_dependencies(${test_target} clear_${test_target}_gcda)
endforeach()
```

综合这一点和前一点的要求

```CMake
add_executable(main main.cpp hello.cpp)

list(APPEND test_targets main)

foreach(test_target IN LISTS test_targets)
  get_property(target_srcs TARGET ${test_target} PROPERTY SOURCES)
  foreach(target_src IN LISTS target_srcs)
    add_custom_command(OUTPUT ./CMakeFiles/${test_target}.dir/clear_${test_target}_${target_src}_coverage.h
    DEPENDS ${target_src}
    COMMAND ${CMAKE_COMMAND} -E echo "clearing ${target_src}.gcno"
    COMMAND ${CMAKE_COMMAND} -E rm -f ./${target_src}.gcno
    COMMAND ${CMAKE_COMMAND} -E touch clear_${test_target}_${target_src}_coverage.h
    WORKING_DIRECTORY ./CMakeFiles/${test_target}.dir
    )
    target_sources(${test_target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${test_target}.dir/clear_${test_target}_${target_src}_coverage.h")

    list(APPEND ${test_target}_gcda ./${target_src}.gcda)
  endforeach()

  add_custom_target(clear_${test_target}_gcda 
  COMMAND ${CMAKE_COMMAND} -E echo "clearing ${${test_target}_gcda}"
  COMMAND ${CMAKE_COMMAND} -E rm -f "${${test_target}_gcda}" 
  WORKING_DIRECTORY ./CMakeFiles/${test_target}.dir
  COMMAND_EXPAND_LISTS)
  
  add_dependencies(${test_target} clear_${test_target}_gcda)
endforeach()
```

## 在VSCODE中配置CMake工程

在`c_cpp_properties.json`中添加

```json
{
    "configurations": [
        {
            "name": "CMake",
            "compileCommands": "${config:cmake.buildDirectory}/compile_commands.json",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

```json
{
    "configurations": [
        {
            "name": "CMake",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

有时`VSCODE`对`C/C++`同时存在的工程有问题，可以把所有`.c`文件改为`.cpp`文件，全部使用`c++`编译器进行编译。
