# CMAKE_EXPORT_COMPILE_COMMANDS

参考文档

* [CMAKE_EXPORT_COMPILE_COMMANDS](https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html#cmake-export-compile-commands)

使能或者失能输出编译命令信息。这个值是目标属性`EXPORT_COMPILE_COMMANDS`的初始值。

如果使能输出命令信息，`CMake`就会把编译选项存储在构建根目录的`compile_commands.json`文件,这个文件的格式如下

```JSON
[
  {
    "directory": "/home/user/development/project",
    "command": "/usr/bin/c++ ... -c ../foo/foo.cc",
    "file": "../foo/foo.cc",
    "output": "../foo.dir/foo.cc.o"
  },

  ...

  {
    "directory": "/home/user/development/project",
    "command": "/usr/bin/c++ ... -c ../foo/bar.cc",
    "file": "../foo/bar.cc",
    "output": "../foo.dir/bar.cc.o"
  }
]
```

这个值只在`Makefile`与`ninja`有效果。

注意，这个只是编译特定源文件为`.o`文件的编译选项，**不包含**将`.o`连接为可执行文件的信息。将`.o`连接为可执行文件的信息存储在存储`.o`文件目录中的`link.txt`文件里。
