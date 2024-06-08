# message

向标准输出或标准错误输出信息

参考文档

* [message](https://cmake.org/cmake/help/latest/command/message.html)

## 命令格式

```CMake
General messages
  message([<mode>] "message text" ...)

Reporting checks
  message(<checkState> "message text" ...)

Configure Log
  message(CONFIGURE_LOG <text>...)
```

## 详细描述

### 输出通用信息

`mode`可以是以下的值

* `FATAL_ERROR`
    `CMake`错误，立即停止CMake运行
* `SEND_ERROR`
    `CMake`错误，继续运行，但不会生成Makefile。
* `WARNING`
    `CMake`警告，继续运行
* `STATUS`
    `CMake`输出信息，默认值
* `VERBOSE`
    `CMake`输出用于工程开发者的信息。
* `DEBUG`
    `CMake`输出用于工程开发者的调试信息。

`message`运行时，会检测对应变量的值决定输出值，比如对于`VERBOSE`,检查`VERBOSE`变量的值，如果`VERBOSE`为1.
