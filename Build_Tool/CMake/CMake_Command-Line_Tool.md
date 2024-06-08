# CMake Command-Line Tool

参考文件

* [CMake Command-Line Tool](https://cmake.org/cmake/help/latest/manual/cmake.1.html#run-a-command-line-tool)

CMake命令行命令是为了跨平台CMake兼容而引入的，可以进行平台无关的命令操作，比如复制文件，删除文件等。

通常是在`add_custom_target()`命令中作为`COMMAND`关键字后面的参数。

## 格式

```shell
cmake -E <command> [<options>]
```

在命令行输入上述格式，就会开启CMake命令行工具，很多工具于linux下的命令用法一致。

* `cat [--] <files>`打印文件到标准输出
* `chdir <dir> <cmd> [<arg>...]`改变当前目录并运行命令
* `compare_files [--ignore-eol] <file1> <file2>`比较文件是否相同
* `copy <file>... <destination>, copy -t <destination> <file>...`复制文件
* `copy_directory <dir>... <destination>`复制目录的所有内容到新的地方，自动创建`<destination>`
* `copy_if_different <file>... <destination>`要是文件已经改变就复制
* `echo [<string>...]`打印字符串到标准输出
* `rename <oldname> <newname>`重命名
* `rm [-rRf] [--] <file|dir>...`删除
* `touch <file>...`创建文件,如果文件已创建，修改它的`access`和`modification`时间。
* `make_directory <dir>..`创建文件夹
