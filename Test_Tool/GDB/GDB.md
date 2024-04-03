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

## 运行程序

* `run`,`r`
  
  开始在GDB里运行程序，如果GDB运行在支持进程的环境，`GDB`就会创建一个进程，运行对应的程序。
  
  可以给`run`指定参数，这个参数会传递给运行的程序。

  程序会继承`GDB`环境变量，可以使用`set environment`和`unset environment`去改变传递给程序的环境变量。

  可以通过命令`set cwd`改变程序运行的目录，如果不设置的话，程序默认继承`GDB`的运行目录。

* `start`

  在主函数开始处打上断点，并开始运行程序。一些程序会在主函数前运行初始化函数，比如构造全局变量，初始化栈等。

* `starti`

  在第一个指令处打上断点，并开始运行程序。

* `kill`

  终止程序的运行。

## 设置程序快照

在Linux上，GDB支持保存程序快照，称为检查点(checkpoint),程序可以返回这里。

程序返回检查点会取消自检查点开始程序所有的修改，包括内存，寄存器，甚至系统状态，就像回到了检查点处重新运行程序一样。

* `checkpoint`

  保存程序当前快照，每个程序快照会和断点一样，被分配一个数字id。

* `info checkpoint`

  列出当前保存的所有程序快照。

* `restart checkpoint-id`

  程序返回到指定快照的状态，所有的变量，寄存器，栈帧都会恢复为原来的状态。

* `delete checkpoint checkpoint-id`

  删除之前保存的程序快照。

返回检查点，还会恢复大部分的系统状态，比如文件指针，但是不会修改已经写入的文件。

## 暂停程序

程序会因为运行到了断点，收到了信号，或者特定的GDB指令后暂停，我们便可以观察暂停程序的状态，发现代码的问题。

* `info program`

  显示目前程序信息，是否正在运行，以及为什么停止。

### 断点

断点分为三大类，断点(Breakpoints)，观测点(Watchpoints)，捕获点(Catchpoints)。

程序运行到断点时便会暂停，还可以添加断点条件。

观测点是特殊的断点，它会在指定表达式的值发生改变时停止程序。

捕获点也是特殊的断点，会在程序发生特定事件是停止程序，比如C++丢出异常，或者加载动态库等。

GDB会给断点分配一个数字id，之后可以用这个id指定特定的断点。每个断点也可以独立使能和失能。

如果指明添加断点的对象不只有一处，比如C++模板函数，内联函数，重载函数等，GDB就会给每个位置分配一个位置id。不能单独删除一个位置的断点，但是可以单独失能它。

GDB提供了一系列方便的变量提供断点的信息，`$bpnum`记录了最近设置的断点的id，`$_hit_bpnum`和`$_hit_locno`分别表示当前碰到的断点id和断点位置id。

* `break locspec`
  
  在`locspec`处设置断点，`locspec`可以是函数名，代码行号，指令的地址等。

  `locspec`可以是如下的格式

  * `Linenum`
  * `+/-offset`
  * `filename:linenum`
  * `function`
  * `function:label`
  * `filename:function`

* `break`

  当没有参数时，`GDB`在选定的栈帧的下一个要被执行的指令处设置断点。在除了最内层以外，这个命令会使得程序在回到这一层的那一刻停止，和指令`finish`的效果类似；在最内层，程序会在下一次它运行到当前位置时停止，（通常用在循环）。

* `break ... if cond`

  设置一个条件断点，当程序运行到这个断点时，首先判断是否`cond`为非零值，若是，则停止程序。如果在特定位置上条件无效，那么`GDB`就会失能这个位置的断点设置。

  ```shell
  (gdb) break func if a == 10
  warning: failed to validate condition at location 0x11ce, disabling:
    No symbol "a" in current context.
  warning: failed to validate condition at location 0x11b6, disabling:
    No symbol "a" in current context.
  Breakpoint 1 at 0x11b6: func. (3 locations)
  ```

* `break ... -force-condition if cond`

  和上一个指令效果相同，只不过会强制设置条件。

* `tbreak args`

  设置一个一次性断点，程序在这个断点停止时，`GDB`会自动删除这个断点。`args`和`break`能接受的参数一样。

* `rbreak regex`

  给所有满足正则表达式`regex`的函数设置断点。当这些断点被设置之后，它们就和普通的断点没什么区别了。

* `rbreak file:regex`

  给文件`file`里的满足正则表达式`regex`的函数设置断点。

* `info breakpoints list...`
  
  显示当前设置的断点类型与相应的信息。

### 观测点

可是设置一个观测点，这个观测点会在指定的表达式的值发生改变时停止程序，不必猜测这个修改发生在代码的那个位置。（这个也可以叫做数据断点(data breakpoint).表达式可以是简单的变量的值，也可以是复杂的表达式，结合C++运算符重载。

取决于操作系统，观测点可以是软件实现或者是硬件实现，软件实现下，程序运行速度会大幅减慢。

* `watch [-l|-location] expr [thread thread-id] [mask maskvalue]`

  为表达式`expr`设置观测点，程序会在表达式被写入与它的值被改变时停止程序。

  如果指定了`[thread thread-id]`，那么GDB会在指定线程id改变表达式的值时停止程序，其它线程改变表达式不会停止程序、

  通常来说，观测点会遵循表达式里变量的作用域，但如果指定了`-location`，就会告诉`GDB`观测表达式指向的地址。在这种情况下，GDB会计算表达式的值，取结果的地址，观测在这个地址的内存。但如果表达式的结果不指向一个内存地址，那么就会报错。

* `rwatch [-l|-location] expr [thread thread-id] [mask maskvalue]`

  设置一个观测点，这个观测点会在程序试图读取表达式时停止。

* `awatch [-l|-location] expr [thread thread-id] [mask maskvalue]`

  设置一个观测点，这个观测点会在程序试图读取或写入表达式时停止。

* `info watchpoints [list…]`

  打印目前设置的观测点

观测一个复杂的表达式可能会耗尽系统允许设置的硬件观测点。

### 捕获点

我们可以设置捕获点捕获特定的程序事件，比如C++异常，动态库加载，系统调用等。

### 删除断点

* `clear`

  删除在选定栈帧下一个指令的所有断点。当最内层栈帧被选择了，就会删除程序刚刚停止在的那个断点。

* `clear locspec`

  删除符合`locspec`的断点。

* `delete [breakpoints] [list…]`

  删除指定的断点，观测点，捕获点。如果没有指定参数，那么就会删除全部断点。

### 使能失能断点

一个断点可以有以下的状态，使能状态，失能状态，使能一次状态，使能N次状态，使能后删除状态。

* `disable [breakpoints] [list…]`

* `enable [breakpoints] [list…]`

* `enable [breakpoints] once list…`

* `enable [breakpoints] count count list…`

* `enable [breakpoints] delete list…`

### 断点条件

