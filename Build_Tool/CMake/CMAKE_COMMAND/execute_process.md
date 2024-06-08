# execute_process

产生一个或者多个子进程，并执行对应的命令。

参考文档

* [execute_process](https://cmake.org/cmake/help/latest/command/execute_process.html#execute-process)

## 命令格式

```CMake
execute_process(COMMAND <cmd1> [<arguments>]
                [COMMAND <cmd2> [<arguments>]]...
                [WORKING_DIRECTORY <directory>]
                [TIMEOUT <seconds>]
                [RESULT_VARIABLE <variable>]
                [RESULTS_VARIABLE <variable>]
                [OUTPUT_VARIABLE <variable>]
                [ERROR_VARIABLE <variable>]
                [INPUT_FILE <file>]
                [OUTPUT_FILE <file>]
                [ERROR_FILE <file>]
                [OUTPUT_QUIET]
                [ERROR_QUIET]
                [COMMAND_ECHO <where>]
                [OUTPUT_STRIP_TRAILING_WHITESPACE]
                [ERROR_STRIP_TRAILING_WHITESPACE]
                [ENCODING <name>]
                [ECHO_OUTPUT_VARIABLE]
                [ECHO_ERROR_VARIABLE]
                [COMMAND_ERROR_IS_FATAL <ANY|LAST>])
```

## 详细描述

产生一个或多个子进程，并执行对应的命令。

`COMMAND`就是子进程命令行要执行的命令，可以输入参数`arg`。

`WORKING_DIRECTORY`表示子进程运行时的目录。可以理解为先运行了`cd`命令跳转到指定的目录。

`TIMEOUT`设置子进程最长运行时间，超时自动终止子进程，同时`RESULTS_VARIABLE`为`timeout`

`RESULT_VARIABLE`包含最后一个子进程输出的结果

`RESULTS_VARIABLE`以一个列表包含所有子进程输出的结果

`INPUT_FILE`表示在第一个命令的输入管道的文件，比如`command1 < filename`

`OUTPUT_FILE`表示在最后一个命令的输出管道的文件，比如`command > filename`

`ERROR_FILE`表示与标准错误管道连接的文件。

`OUTPUT_QUIET`, `ERROR_QUIET`表示禁止输出或者错误到`OUTPUT_VARIABLE`或`ERROR_VARIABLE`

`OUTPUT_VARIABLE`, `ERROR_VARIABLE`表示标准输出和标准错误存储到的变量。

`COMMAND_ECHO`表示命令会回显到`where`指示的输出，`where`可以是`STDERR`,`STDOUT`,`NONE`