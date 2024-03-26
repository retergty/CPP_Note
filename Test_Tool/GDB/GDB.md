# GDB

GDB(GNU Debugger)是GNU项目的调试器，可以调试使用GNU工具链编译出的可执行文件。

参考文档

* GNU官方文档[Debugging with GDB](https://sourceware.org/gdb/download/onlinedocs/gdb.html/index.html)

GDB有四种功能来捕获程序中的错误。

* 开始运行程序，指定任何可能影响其行为的内容
* 使程序在特定条件下暂停
* 当程序暂停时检查发生了什么
* 更改程序的内容

GDB支持多种语言，但是我们通常用它来调试`C`或`C++`程序。

## 编译可用于GDB的可执行文件

为了能够使用`GDB`进行调试，我们在编译可执行文件时需要指定`-g`生成调试信息，通常不指定`-O`，因为会带来麻烦的调试，但是`GCC`允许指定`-g`加上`-O`.

## 启动GDB

```shell
gdb program
```

使用`GDB`加上可执行文件名就可以启动gdb，之后就会打开gdb命令行，我们可以在这里输入gdb指令。

```shell
gdb --args gcc -O2 -c foo.c
```

我们可以使用`GDB`给可执行文件传递参数，上面的命令调试`gcc`并给它传递了参数`-O2 -c foo.c`.

## 退出GDB

```shell
quit [expression]
exit [expression]
q
```

上面三个指令都可以退出`GDB`，如果指定了`expression`，那么就会返回这个`expression`

`ctrl+c`组合键不会退出GDB，而是终止目前正在运行的`GDB`指令并回到`GDB`命令行。

## GDB指令

GDB支持一系列的指令，用来测试程序，打断点，打印变量值，跳转定义等。

### GDB指令格式

`GDB`指令由单行组成，不限制输入字符个数，可以接受参数（参数含义取决于具体的指令）。

我们可以缩写GDB指令，只要这个缩写没有歧义。

使用回车键(RET)重复上一条指令，或者使用`TAB`键在`GDB`命令行中填充上一条指令。

对于特定指令，比如`list`，如果使用回车键重复的话，会构建新的参数而不是完全重复之前的参数，这个特性方便我们调试。

任何在`#`后面的内容都被认为是注释，`GDB`会忽略这些内容。

### GDB指令设置

许多指令会根据对应的变量或设置改变它的行为。我们用一个特殊的`GDB`指令`set`来修改`GDB`指令设置。

这些设置可以在`GDB`启动时，通过文件来修改，也可以在`GDB`运行时通过`set`修改。

```shell
(gdb) set print elements 10
(gdb) print some_array
$1 = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90...}
```

比如，通过`set`把`print`打印的数组元素个数从默认的200修改为10.

当然，`print`也可以通过参数修改打印的个数。

```shell
(gdb) print -elements 10 -- some_array
$1 = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90...}
```

我们也可以通过`with`指令临时修改设置。

```shell
with setting [value] [-- command]
w setting [value] [-- command]
```

临时修改设置并运行命令`command`，没有指定`command`就为上一个运行的命令。

```shell
(gdb) with print array on -- print some_array
```

### GDB指令补全

GDB具有强大的指令补全功能，补全方法和`shell`一样，在想要补全的地方键入`TAB`就行了。也可以通过`ESC+?`的组合键，显示所有可能的补全。

有时我们想要补全的单词中可能含有分隔符，比如模版实例化`<>`，为了对这种单词进行补全，我们有时需要用单引号将单词括起来。

```shell
(gdb) p 'func<M-?
func<int>()    func<float>()
(gdb) p 'func<
```

## 常用GDB指令

### 运行程序

