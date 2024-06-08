# add_test

在工程中加入一个测试文件，可以被`ctest`运行。

参考文档

* [add_test](https://cmake.org/cmake/help/latest/command/add_test.html#command:add_test)

## 命令格式

```CMake
add_test(NAME <name> COMMAND <command> [<arg>...]
         [CONFIGURATIONS <config>...]
         [WORKING_DIRECTORY <dir>]
         [COMMAND_EXPAND_LISTS])
```

## 详细描述

加入一个名为`name`的测试，

只有当命令`enable_testing()`已经调用，CMake才会生成测试。

`COMMAND`声明这个测试运行时的命令行命令，如果涉及到使用`add_executable()`创造的可执行文件目标名`tgtname`，那么就会自动替换为`build`阶段创建的可执行文件的完整路径。支持生成期表达式。

`WORKING_DIRECTORY`设置测试属性`WORKING_DIRECTORY`，测试会在`WORKING_DIRECTORY`文件夹运行，如果没有指定，那么默认为`CMAKE_CURRENT_BINARY_DIR`

如果测试命令`COMMAND`返回值不为零，测试失败。但是测试属性`WILL_FAIL`会反转结果。但是，系统级别的错误，比如`segmentation faults`或者是`heap errors`无视`WILL_FAIL`.`ctest`程序捕捉测试输出的标准输出和标准错误，并与`PASS_REGULAR_EXPRESSION`,`FAIL_REGULAR_EXPRESSION`,`SKIP_REGULAR_EXPRESSION`（若设置），确定最终测试的结果。

```CMake
add_test(NAME mytest
         COMMAND testDriver --config $<CONFIG>
                            --exe $<TARGET_FILE:myexe>)
```

创建一个名为`mytest`的测试，运行`testDriver`工具，接受参数`--config $<CONFIG> --exe $<TARGET_FILE:myexe>`.