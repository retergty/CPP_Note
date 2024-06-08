# ctest

集成在CMake里的代码测试工具，通过`CMakeLists.txt`命令`enable_testing()`与`add_test()`可以创建测试，`ctest`工具就可以使用这些测试，来测试代码运行的正确性，达到自动化测试的目的。

参考文档

* CMake 官方文档[ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

## 用法

```shell
Run Tests
 ctest [<options>] [--test-dir <path-to-build>]

Build and Test Mode
 ctest --build-and-test <path-to-source> <path-to-build>
       --build-generator <generator> [<options>...]
      [--build-options <opts>...]
      [--test-command <command> [<args>...]]

Dashboard Client
 ctest -D <dashboard>         [-- <dashboard-options>...]
 ctest -M <model> -T <action> [-- <dashboard-options>...]
 ctest -S <script>            [-- <dashboard-options>...]
 ctest -SP <script>           [-- <dashboard-options>...]

View Help
 ctest --help[-<topic>]
```
