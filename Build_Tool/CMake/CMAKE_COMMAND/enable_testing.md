# enable_testing

在当前文件夹与子文件夹下使能测试

参考文档

* [enable_testing](https://cmake.org/cmake/help/latest/command/enable_testing.html#command:enable_testing)

## 命令格式

```CMake
enable_testing()
```

## 详细描述

在当前文件夹与子文件夹下使能测试，这个命令应该放在源文件根目录中，因为`ctest`期望在构建根目录下找到测试文件。
