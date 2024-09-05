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

## 概念

### inferior

`GDB`使用一个叫做`inferior`的对象表示每个程序的运行状态。一个`inferior`通常就是一个进程。也就是说`inferior`就是我们要`debug`的程序。

## 编译可用于GDB的可执行文件

为了能够使用`GDB`进行调试，我们在编译可执行文件时需要指定`-g`生成调试信息，通常不指定`-O`，因为会带来麻烦的调试，但是`GCC`允许指定`-g`加上`-O`.

## 目标target

目标就是Debug程序占用的执行环境。

## 启动GDB

```shell
gdb program
```

使用`GDB`加上可执行文件名就可以启动gdb，之后就会打开gdb命令行，我们可以在这里输入gdb指令。

```shell
gdb --args gcc -O2 -c foo.c
```

我们可以使用`GDB`给可执行文件传递参数，上面的命令调试`gcc`并给它传递了参数`-O2 -c foo.c`.

也可以在`run`命令时指定参数。

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

## 指定Debug可执行文件

有时，需要在`GDB`命令行里指定可执行文件。

* `file filename`

  指定要`debug`的可执行文件名，`GDB`会读取它的符号表，之后可以通过`run`执行这个可执行文件。如果指定的可执行文件没有在`GDB`当前工作目录被找到，那么`GDB`会使用环境变量`PATH`指定的目录查找。

  如果不指定参数，`GDB`会丢弃当前存储的符号表与可执行文件信息。

* `exec-file [ filename ]`

  指定要执行的可执行文件名，但不读取它的符号表。

* `symbol-file [ filename [ -o offset ]]`

  读取`filename`指定文件的符号表信息

* `info files`
* `info taget`

  显示当前`debug`的目标或者是文件信息。

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

* `attach process-id`
  
  这个命令把一个正在运行的进程联系到`GDB`,

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

可以为断点设置条件，条件是一个布尔表达式，每次程序达到断点时，就会检测条件，为真时程序停止。

断点条件可以有副作用，也可以调用程序内的函数。

可以直接在设置断点时使用`if`关键字，也可以之后指定条件。

* `condition bnum expression`

  声明`expression`为断点条件，`GDB`会检查`expression`的语法错误，当`expression`使用了一个没有存在于对应断点的上下文中的符号，`GDB`打印一个错误消息。

* `condition -force bnum expression`

  当使用`-force`参数时，哪怕表达式**在任何位置的断点上都无效**也会定义断点条件。

* `condition bnum`

  移去对应断点的条件。

有一种特殊的断点条件是，当这个程序达到断点第`n`次时程序才停止。每个断点都有一个特殊的计数变量叫做`ignore`。如果`ignore`大于零，当程序运行到断点时不会停止，而是给`ignore`减去1.直到`ignore`减到零，下次的断点才会停止。

* `ignore bnum count`

  设置`ignore`为`count`。

### 断点命令

我们可以给特定断点设置一系列的命令，这些命令会在程序由于这个断点**停止**时运行。比如我们可以在停止时打印数据，或者使能另一个断点。

* ```text
  command [list]
  ... command-list ...
  end
  ```

  给指定的断点声明命令，这些命令是任意的`GDB`命令，每个命令间以回车分隔，最后一行输入`end`结束命令。

  如果想要删除指定断点的命令，输入一个空白的命令就行。

  如果没有指定参数，`command`默认为**最近设置**的命令。

## 继续运行与步进(Continuing and Stepping)

继续运行意味着从停止处继续运行程序；步进意味着从停止处运行一步后又停止，一步可以是代码里的一行，也可以是一行机器代码。无论是继续运行还是步进，程序都有可能提前停止，比如遇到了一个新的断点(breakpoint)或者是信号(signal)。

* `continue [ignore-count]`,`c [ignore-count]`,`fg [ignore-count]`

  在上次停止的程序地址处继续运行，忽略在这个地址上的断点，以免重复停止，`ignore-count`还可以指定这些断点的忽略计数。

* `step`

  继续运行程序直到达到不同的源代码行，之后程序停止。

  如果在`step`运行时遇到了没有调试信息的函数，程序会继续运行，直到遇到了存在调试信息的函数。

  `step`只会停止在源代码行的**第一条指令**处，不会在源代码行的中间处停止、

  `step`会进入这一行中的任何函数并停止。

* `step count`

  类似于运行`step`命令`count`次，但是如果运行中碰到了断点或者是信号，程序立即停止。

* `next [count]`

  继续运行程序直到达到当前栈帧不同的代码行，这个命令和`step`类似，只不过它不会停止在这一行的内部函数。

  `count`参数和`step`中`count`参数作用相同。

* `finish`

  继续运行程序直到选定的栈帧返回，停止在栈帧返回点的函数后，打印返回值（如果有）。

* `until`,`u`

  继续运行程序直到达到了当前栈帧新的代码行。`until`命令和`next`类似，只不过当`until`遇到了`jump`后，它会继续运行直到`pc`指针比`jump`地址要大。这意味着，如果我们在循环末尾处使用`until`时，程序会一直运行直到循环退出。与之对应，`next`命令则会回到循环的第一个指令处停止。

  如果`until`命令尝试退出当前的栈帧时，它会停止。

* `until locspec`,`u locspec`

  继续运行程序，直到它达到了`locspec`，或者当前栈帧返回。这个命令使用临时断点。这个暗示了，`until`可以使用在跳过递归函数的调用。比如对于如下的代码片段，使用`until 99`就可以跳过所有的内部递归函数。

  ```CPP
  int factorial (int value)
  {
    if (value > 1) {
    value *= factorial (value - 1);
  }
    return (value); // line 99
  }
  ```

* `advance locspec`

  继续运行程序直到它达到了`locspec`或者是当前栈帧返回，这个命令和`until`类似，但是`advance`不会跳过递归函数的调用。

### 跳过函数与文件

程序可能会包含一些不感兴趣的函数，我们可以使用`skip`告诉`GDB`跳过这个函数。

```CPP
int func()
{
foo(boring()); // line 103
bar(boring());
}
```

假如我们想要进入`foo`但不想进入`boring`，如果在103行使用命令`step`，程序会进入`boring`。使用`next`的话，程序又会直接跳过`foo`.

一个解决方法是，使用`step`后接着使用`finish`。

更灵活的方法是使用`skip boring`，告诉`GDB`永远不要进入到`boring`中去。现在再调用`step`的话，就不会进入`boring`中了。

* `skip [options]`

  普通格式的`skip`接受零个或者多个参数，指明要跳过什么、`options`如下

  * `-file file`,`-fi file`

    在`file`中的函数会被跳过。

  * `-gfile file-glob-pattern`,`-gfi file-glob-pattern`

    匹配包含通配符的`pattern`字符串的文件中的函数会被跳过。
  
  * `-function linespec`,`-fu linespec`

    函数由`linespec`命名或者是函数包含以`linespec`命名的行会被跳过。
  
  * `-rfunction regexp`,`-rfu regexp`

    函数匹配正则表达式`regexp`会被跳过。

    这个方法对于`C++`的模板函数很友好，我们可以不指定模版实参。

    比如跳过`C++`标准库的构造或者是析构函数

    ```gdb
    skip -rfu ^std::([a-zA-z0-9_]+)<.*>::~?\1 *\(
    ```

  如果没有指定参数，跳过当前的函数。

* `skip function [linespec]`

  和上述一样

* `skip file [filename]`

  和上述一样

* `info skip [range]`

  打印目前设置的跳过对象。指定`range`后打印`range`范围的跳过对象。

* `skip delete [range]`

  删除指定的跳过，如果没有指定`range`删除所有跳过。

* `skip enable [range]`

  使能指定的跳过

* `skip disable [range]`

  失能指定的跳过。

### 信号(Signals)

一个信号是一个异步事件，操作系统会定义可能的信号种类，并给每个信号一个信号名和编号。比如`SIGINT`会在我们键入中断符时触发。

一些信号，比如`SIGALRM`是程序的正常行为。但是一些信号，比如`SIGSEGV`表示的是程序错误。这些信号是致命的，如果程序没有写明如何处理这种信号的话，操作系统会立即杀死程序。`SIGINT`通常不是程序的错误，但它经常来说却是致命的，它会直接杀死程序。

`GDB`有检测程序收到的信号的能力，还有定义收到信号后如何做的能力。

通常，`GDB`设置为对于非错误的信号安静地跳过，对于致命信号则立即停止程序。我们可以通过`handle`命令改变设置。

* `info signals`,`info handle`

  打印所有类型的信号以及`GDB`如何处理这个信号，用这个命令可以得到信号的编号。

* `info signals sig`

  类似上述命令，但是只打印编号为`sig`的信号。

* `catch signal [signal… | ‘all’]`

  给指定的信号设置一个观测点。

### 多线程

`GDB`支持调试多线程程序，有两种调试多线程程序的模式。默认模式下，也就是全部停止模式(all-stop mode),当任意一个线程停止时，其它所有的线程也会停止。还有另一种模式，叫做不停止模式(non-stop mode)，其它线程据需运行。

#### 全部停止模式

在全部停止模式下，当程序由于`GDB`原因而停止时，所有的线程都会停止，这个模式允许我们自由地切换线程，不用担心数据改变。

相对应的，当重新运行程序时，所有的线程都会开始运行，哪怕是那些单步调试的命令，比如`step`,`next`,

特别的，使用单步调试命令时，`GDB`并不是单步运行所有线程。因为线程调度是取决于`GDB`目标的操作系统的，不由`GDB`控制，当当前线程完成一步时，别的线程可以会运行多于一步。而且，当程序停止时，别的线程可能会停在语句的中间，而不是开头。

程序也有可能停在别的线程，这个发生在当别的线程达到断点，收到信号或者是遇到异常。

任何时候当`GDB`由于断点和信号停止了程序，它会自动地选择遇到了断点和信号的线程。`GDB`会打印一个消息提醒我们发生了线程切换，类似于`[Switching to Thread n]`.

在某些操作系统上，我们可以改变`GDB`的默认行为，通过锁定操作系统调度器去只允许单个线程运行。

* `set scheduler-locking mode`

  设置调度锁模式。`mode`可以是如下的值

  * `off`

    关闭调度锁，所有线程自由运行。

  * `on`

    只有当前的线程可以运行

  * `step`

    当步进时和`on`的行为一样，其他情况下和`off`行为一样。

  * `replay`

    在`replay`模式下和`on`行为一样，其他模式下和`off`行为一样。

默认情况下，当使用`GDB`命令时，`GDB`只允许当前进程的线程运行。比如`GDB`连接到两个进程，每个进程有两个线程，当使用`continue`命令时，只会恢复当前进程的两个线程。

* `set schedule-multiple`

  指示`GDB`允许多进程的线程恢复运行，当`on`时，所有进程的所有线程都可以运行。当`off`时，只有当前线程的进程可以运行。默认是`off`。设置`scheduler-locking`优先于这个设置。

* `show schedule-multiple`

  展示当前设置。

### 设置具体线程断点

当程序有多个线程时，可以选择设置一个特定线程的断点。

* `break locspec thread thread-id`,`break locspec thread thread-id if …`

  设置特定线程的断点，只有这个线程遇到这个断点时，程序才停止。`thread-id`是`GDB`分配给每个线程的`id`，可以使用`info threads`查看。

  具体线程断点会在对应线程不再存在时自动删除。

### 中断的系统调用

使用`GDB`调试多线程程序时有一个不幸的副作用。如果一个线程因断点或其他原因而停止，而另一个线程在系统调用中被阻塞，则系统调用可能会提前返回.这是多个线程和`GDB`用于实现断点和其他停止执行的事件的信号之间交互的结果。

注意，是系统调用提前返回，而不是系统调用失败，几乎所有的阻塞系统调用都规定了系统调用可能提前退出，所以用户本来就需要检测是否完成系统调用，这是一个良好的编程习惯。

比如,`sleep`系统调用

```C++
int unslept = 10;
while (unslept > 0)
  unslept = sleep (unslept);
```

系统调用允许提前返回，因此系统仍然符合其规范。但`GDB`确实会导致多线程程序的行为与没有`GDB`时不同。

## 反向运行程序

有时在调试程序时，不可避免地发现程序执行得太远了，超过了感兴趣的程序点。如果目标环境支持，`GDB`可以把程序倒回去。

反向运行程序要求取消之前设置的变量，寄存器值，这个显然是十分困难的，以至于不是每个运行环境都支持这个操作。

* `reverse-continue [ignore-count]`,`rc [ignore-count]`

  从上次程序停止处，反向运行程序，反向运行会在遇到断点或者是信号时停止。

* `reverse-step [count]`

  和`step`类似，只不过是反向运行。

* `reverse-next [count]`

  和`next`类似，只不过是反向运行。

* `reverse-finish`

  和`finish`类似，只不过是反向运行。

## 记录程序运行并重播

在某些平台上，`GDB`提供了一个特别的目标记录进程的运行日志，并可以在稍后正反双向重放。

当这个目标使用时，且处理日志中包含了下一行机器指令，那么`GDB`会以`replay`模式进行调试。当在`replay`模式中运行时，进程不会真的执行代码，相反，代码执行期间通常发生的所有事件都从执行日志中获取。尽管代码实际没有在`replay`模式中运行，但寄存器的值，内存值都会改变，这些改变值是从运行日志中获取的。

如果处理日志中不包含下一行机器指令，那么`GDB`就会以`record`模式运行，在这个模式中，进程正常运行，`GDB`记录运行结果到处理日志中。

进程记录与重放支持反向运行，尽管当前操作系统不支持反向运行。但是，反向运行的范围限制在运行日志所记录的范围中。

* `record method`

  这个命令开启进程记录与重放目标，可以指定记录方法。默认的方法是`full`，可用的记录方法如下。

  * `full`
  
    使用`GDB`的软件完全记录信息，这个方法允许重放和反向运行。

  * `btrace format`

    英特尔处理器硬件支持的机器指令记录方法。

  记录命令只可以记录一个已经在运行的进程,所以运行命令`record`前，需要运行命令`start`或者`run`.

* `record stop`

  停止当前记录目标的运行。当记录目标停止时，整个处理日志会被删除，被记录的进程要不停止，要不保持在最后的状态。

  当在`record`模式下停止了记录目标时，被记录的进程会停止在下一条本应被记录的机器指令处。换句话说，当记录目标停止时，被记录的进程会处于好像记录从来没有开始过的状态中。

  而在`replay`模式下停止了记录目标时，被记录的进程之后可能会继续调试。

* `record goto`

  跳转到处理日志的特殊位置，以下是可能的位置

  * `record goto begin`,`record goto start`

    跳转到处理日志的开头。

  * `record goto end`

    跳转到处理日志的末尾

  * `record goto n`

    跳转到处理日志的第`n`行

* `record save filename`

  保存处理日志到文件`filename`里，默认文件名为`gdb_record.process_id`.

* `record restore filename`

  从文件`filename`中获得处理日志。

* `set record full insn-number-max limit`,`set record full insn-number-max unlimited`

  设置处理日志可以保存的最多指令数，默认是200000.

  如果`limit`是正数，`GDB`会在达到记录的指令数时，删除最早记录的指令。

  如果`limit`是`unlimited`或是`0`,`GDB`会尽可能地保存更多的指令，直到内存限制。

* `show record full insn-number-max`

  显示处理日志可以保存的最多指令数。

* `set record full stop-at-limit`

  控制当处理日志达到最多指令数时的行为，为`ON`是程序停止，为`OFF`时，删除最早记录的指令。

* `show record full stop-at-limit`

  显示当处理日志达到最多指令数时的行为。

* `info record`

  显示当前记录的处理日志的信息

* `record delete`

  当记录目标运行在`replay`模式时，删除接下来的处理日志，并从当前地址开始记录新的处理日志。

## 栈

当程序停止时，首先要知道的就是程序在哪里停止的以及它是怎么来到这的。

每次程序调用函数时，都会生成有关这个调用的信息，这些信息包括程序调用这个函数的位置，传递的实参，被调用函数的本地变量，这些信息会被保存在一块数据中，这块数据叫做栈帧(stack frame)，栈帧是在一段叫做调用栈(call stack)的内存中分配空间的。

当程序停止时，特定的`GDB`命令可以帮助我们取得栈帧的信息。

当一个栈帧被`GDB`特定命令选择时，许多`GDB`命令会隐式的相对于这个栈帧运行。比如，当打印变量值时，指的就是当前栈帧可见的变量。

当程序停止时，`GDB`自动选择当前正在运行的帧并简单地描述了一下。

### 回溯(Backtraces)

回溯指的是程序如何到达这里的信息总结，每行信息包含一个栈帧，对于多个栈帧，开始于当前正在运行的栈帧(frame zero)，接着是调用者(frame one)，以此类推。

使用`backtrace`打印回溯信息，默认情况下，所有的栈帧都会被打印。

* `backtrace [option]… [qualifier]… [count]`,`bt [option]… [qualifier]… [count]`

  打印整个回溯信息。

  `count`可以指示要打印多少个栈帧，为正数时,打印`count`个最内层栈帧；为负数时，打印`count`个最外层栈帧。

  `option`可以是

  * `-full`

    打印所有本地变量的值。

  * `-no-filters`

    不要运行`Python`栈帧过滤。

  `backtrace`命令还支持一系列的命令用来覆盖全局打印回溯信息的参数。可参考`GDB`官方文档。

`where`和`info stack`是`backtrace`的别名。

对于多线程程序，`GDB`默认只打印当前线程的回溯信息，为了显示别的线程的回溯信息，使用`tread apply all backtrace`显示所有线程的回溯信息。

回溯信息的每一行都展示了一个栈帧，包含栈帧编号，函数名，pc指针，源代码文件名，行号以及函数实参。如果该栈帧是源代码行中的开始，则忽略pc指针。

比如`bt 3`会显示如下信息

```text
#0  m4_traceon (obs=0x24eb0, argc=1, argv=0x2b8c8)
    at builtin.c:993
#1  0x6e38 in expand_macro (sym=0x2b600, data=...) at macro.c:242
#2  0x6840 in expand_token (obs=0x0, t=177664, td=0xf7fffb08)
    at macro.c:71
(More stack frames follow...)
```

栈帧1的实参`data`被`...`替换了，这是因为，默认情况下`GDB`只会打印数值类型的实参。

如果使用了优化编译，一些编译器可能会优化掉传递的实参，这些优化直接使用寄存器传递实参，没有把实参存储在栈帧中，`GDB`没有办法在不是最内层的栈帧上显示这些实参。

通常境况下，`GDB`会在发现`main_`程序入口点时停止接下来的栈帧回溯，因为之后的栈帧都是系统级别的调用了。

* `set backtrace past-main [on|off]`

  栈帧回溯是否在`main_`停止。

* `set backtrace past-entry [on|off]`

  栈帧回溯是否在入口点停止，这个入口点是链接器生成的，再往外便是系统级别的调用了。

* `set backtrace limit n`

  设置最多打印栈帧的数量，为`0`和`unlimited`时，无限制。

* `set filename-display [relative|basename|absolute]`

  设置显示文件名的方法，相对于当前的编译目录，还是只显示目录，还是显示绝对地址。

### 选择栈帧

几乎所有的有关打印栈信息的`GDB`命令的行为都与当前选择的栈帧有关。

* `frame [frame-selection-spec]`,`f [frame-selection-spec]`

  选择指定的栈帧。

  `frame-selection-spec`可以是如下的值

  * `num`,`level num`

    栈帧数字，从零开始，可以使用`backtrace`查看。
  
  * `adresss stack-address`

    通过栈地址选择栈帧

  * `function function-name`

    通过函数名选择栈帧，如果多个栈帧都有同一函数名，则选择最内层的。

* `up n`

  向上选择当前的第`n`的栈帧，默认为1，方向是从小栈帧编号到大栈帧编号

* `down n`

  向下选择当前的第`n`的栈帧，默认为1，方向是从大栈帧编号到小栈帧编号

上面的命令都会输出两行来描述选择的栈帧，第一行是栈帧编号，函数名，实参，源文件以及对应的行号。第二行显示了源文件对应行号的代码。

```text
(gdb) up
#1  0x22f0 in main (argc=1, argv=0xf7fffbf4, env=0xf7fffbfc)
    at env.c:10
10              read_input_file (argv[i]);
```

### 栈帧信息

有几种命令可以打印选择的栈帧信息

* `frame`,`f`
  
  当不指定参数时，这个命令不会切换栈帧，而是打印当前栈帧的简短信息。

* `info frame [frame-selection-spec]`,`info f [frame-selection-spec]`

  这个命令打印选定栈帧的详细信息，包括栈帧地址，调用和被调用的栈帧地址等。没有指定参数时则是当前栈帧。

* `info args`

  打印当前栈帧的实参。

* `info locals [-q] [-t type_regexp] [regexp]`

  打印当前栈帧的本地变量。这些变量是当前栈帧处理点可以访问的所有变量。

  `-t type_regexp`指定时，只打印类型满足该正则表达式的本地变量。

  `regexp`指定值，只打印名字满足该正则表达式的本地变量。
  
### 将命令应用到复数个栈帧

* `frame apply [all|count|-count|level level...] [option]... command`

  将命令`command`应用到多个栈帧。

  `all`指应用到所有栈帧。

  `count`指应用到从最内层开始的`count`个栈帧。

  `-count`指应用到从最外层开始的`count`个栈帧。

  `level`指特定编号的栈帧，比如`level 2-4 6-8 3`.

## 源代码

GDB提供了许多命令方便地查看源代码。

### 打印源代码

使用`list`打印特定的源代码，默认是打印10行。

* `list linenum`

  打印当前源文件行号`linenum`周围的源代码行。

* `list function`

  打印函数`function`周围的源代码行。

* `list`

  打印更多的行，打印的行取决于之前的命令。如果上一次的行打印是使用`list`命令打印的，那么这一个命令就会打印上一次的行后面的源代码行。但是，如果上一次行打印是使用栈帧命令打印的，这个命令就会打印上一次行的周围。如果上述条件都不满足，打印`main`周围的行。

* `list +`

  和`list`一样

* `list -`

  只打印上次打印行的前几行。

* `list .`

  打印当前选择的栈帧周围的源代码。

默认`GDB`打印10行代码，可以通过命令修改

* `set listsize count`,`set listsize unlimited`

  设置`list`打印的行数。

* `show listsize`

  显示`list`默认打印的行数。

使用回车重复`list`命令会丢弃传递给`list`的参数，也就是等同于输入`list`.

* `list locspec`

  打印满足`locspec`名字周围的代码。

* `list first,last`

  打印从`first`到`last`行的源代码。

* `list ,last`

  打印以`last`结尾的行

* `list first,last`

  打印以`first`开头的行

### 指定位置(Location Specifications)

许多`GDB`命令支持指定位置的参数，也就是`locspec`.使用这个来指定诸如源代码行号，函数名，地址，标签等。

#### 指定行号

这种类型的位置指定的是行号位置。

* `linenum`

  指定当前源文件的第`linenum`行。

* `-offset`,`+offset`

  指定当前行的前或后`offset`行。对于`list`命令，当前行指的就是上一次打印的行，或是在使用了两个行号位置时，比如`first,last`时，`last`的当前行就是`first`指定的行；对于`breakpoint`命令，当前行指的就是当前程序停止在的地方。

* `filename:linenum`

  指定文件`filename`的第`linenum`行，如果文件名`filename`是相对路径，那么它就会匹配所有包含这个部分的文件名。比如`gcc/expr.c`就会匹配`/build/trunk/gcc/expr.c`,不匹配`/build/trunk/libcpp/expr.c`和`/build/trunk/gcc/x-expr.c`

* `function`

  指定函数`function`函数体开始的行，比如,对于`C/C++`来说直接就是`{`所在的那一行。

  默认情况下，对于`C++`来说，`function`会被翻译为所有作用域中名字为`function`的函数，也就是所有名称空间和类定义的函数。

  比如`C++`程序中有两个函数，`A::B::func`,`B::func`，使用命令`break func`或`break B::func`会给这两个函数都设置上断点。

  可以使用`-qualified func`指定取设置一个`func`全局函数名，而不是类方法也名称空间函数名。

* `function:label`

  指定出现在`function`里的`label`哪一行

* `filename:function`

  指定在文件`filename`里的函数`function`函数体开始的行。

* `label`

  指定当前栈帧下，函数中名为`label`的标签那一行。

#### 显式指定

使用参数`-source filename`,`-function function`,`-qualified`,`-label label`,`-line number`.可以显式指定位置

#### 指定地址

可以直接指定地址，形如`*address`.

## 显示数据

通常的方法显示数据就是使用命令`print`。这个命令会计算并打印指定的表达式的值。

* `print [[options] --] expr`,`print [[options] --] /f expr`

  `expr`就是当前语言合法的表达式，我们可以指定`/f`来选定打印格式。

  `option`可以是如下的选项

  * `-address [on|off]`

    也打印地址。

  * `-array [on|off]`

    更漂亮地打印数组。

  * `-array-indexes [on|off]`

    也打印数组的下标。

  * `-characters number-of-characters|elements|unlimited`

    设置最多打印的字符数。

  * `-elements number-of-elements|unlimited`

    设置最多打印的数组元素个数和字符串长度。

  * `-max-depth depth|unlimited`

    设置打印结构体类型的最大深度。

  * `-nibbles [on|off]`

    设置是否以四位为一组（称为“半字节”）打印二进制值。

  * `-memory-tag-violations [on|off]`

    设置打印有关内存标签违规的附加信息.

  * `-null-stop [on|off]`

    将字符数组的打印设置为在第一个`null`字符处停止。

  * `-object [on|off]`

    打印`C++`虚函数表

  * `-pretty [on|off]`

    更漂亮地打印结构体。

  * `-raw-values [on|off]`

    是否取消所有的格式化，直接打印原始形式。

  * `-repeats number-of-repeats|unlimited`

    设置重复打印元素的阈值。

  * `-static-members [on|off]`

    设置打印`C++`静态成员。

  * `-symbol [on|off]`

    设置打印指针时打印符号名称。

  由于`print`可以接受任何表达式，所以，如果想指定形如参数的表达式，需要用`--`分割开来。

  如果省略了`expr`，会默认是上一次打印地表达式。

  ```text
  (gdb) print -pretty -- *myptr
  $1 = {
    next = 0x0,
    flags = {
      sweet = 1,
      sour = 1
    },
    meat = 0x54 "Pork"
  }
  ```

* `explore arg`

  `arg`可以是一个表达式，也可以是一个类型，这个命令对于复杂的结构体很有用。

  ```C
  struct SimpleStruct
  {
    int i;
    double d;
  };

  struct ComplexStruct
  {
    struct SimpleStruct *ss_p;
    int arr[10];
  };
  ```

  初始化为

  ```C
  struct SimpleStruct ss = { 10, 1.11 };
  struct ComplexStruct cs = { &ss, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
  ```

  使用`explore`有

  ```text
  (gdb) explore cs
  The value of `cs' is a struct/class of type `struct ComplexStruct' with
  the following fields:

    ss_p = <Enter 0 to explore this field of type `struct SimpleStruct *'>
    arr = <Enter 1 to explore this field of type `int [10]'>

  Enter the field number of choice:
  ```

  我们之后可以输入编号进入结构体成员中。

  总的来说，使用这个命令，可以沿着复杂结构体查找。

### 表达式

`print`等许多`GDB`命令可以接受一个表达式并计算它的值，表达式可以是任何的当前语言合法的表达式，比如条件表达式，函数调用，类型强制转换，字符串常量等。

`GDB`定义了以下的运算符

* `@`

  `@`是一个二元运算符，用于把部分内存视为数组。

* `::`

  `::`允许特别指定一变量的定义在的文件位置或者作用域

* `{type} addr`

  指向一个类型为`type`的对象，这个对象储存在地址`addr`里。`addr`可以是任意的表达式，只要它的值是一个整数。

#### 表达式多义性

对于`C++`来说，一个表达式可以有多种意思，比如函数重载和模版函数情况下，有时我们需要指定具体的表达式。

取决于`GDB`设置，`GDB`会打印一个表格供我们选择，我们输入序号就可以了。

```text
(gdb) b String::after
[0] cancel
[1] all
[2] file:String.cc; line number:867
[3] file:String.cc; line number:860
[4] file:String.cc; line number:875
[5] file:String.cc; line number:853
[6] file:String.cc; line number:846
[7] file:String.cc; line number:735
> 2 4 6
Breakpoint 1 at 0xb26c: file String.cc, line 867.
Breakpoint 2 at 0xb344: file String.cc, line 875.
Breakpoint 3 at 0xafcc: file String.cc, line 846.
Multiple breakpoints were set.
Use the "delete" command to delete unwanted
 breakpoints.
(gdb)
```

* `set multiple-symbols mode`

  这个选项允许我们指定`GDB`当遇到多义的表达式时的行为。

  这个选项默认是`all`,意味着如果表达式是多义的，默认选择所有表达式。比如，插入一个具有重载的函数断点会给所有对应的函数插入断点。但是，如果命令需要一个独一无二的表达式，`GDB`还是会打印一个表格供我们选择。

  当模式是`ask`时，`GDB`总是会询问我们。

  当模式是`cancel`时，`GDB`会对多义表达式报告错误。

#### 变量

最常用的表达式类型就是变量名。

表达式中使用的变量名必须当前栈帧合法且可见的变量名。但有一个例外，可以指定作用域为单个源文件的变量或者函数，无论当前程序运行点是否在这个文件里。如果有多个变量或者函数同名，我们需要使用`file::var`或`function::var`指定变量。

```text
p 'f2.c'::x
```

`::`通常用于指定静态变量，也可以用它来指向最近的含有这个变量的上层栈帧。

```CPP
void
foo (int a)
{
  if (a < 10)
    bar (a);
  else
    process (a);    /* Stop here */
}

int
bar (int a)
{
  foo (a + 5);
}
```

```text
(gdb) p a
$1 = 10
(gdb) p bar::a
$2 = 5
(gdb) up 2
#2  0x080483d0 in foo (a=5) at foobar.c:12
(gdb) p a
$3 = 5
(gdb) p bar::a
$4 = 0
```

当我们在`process(a)`打上断点，并在程序中写下`bar(0)`.我们就可以用`::`指定特定的变量。

有时`::`会与`C++`里作用域解析符`::`冲突，此时，选择`C++`的含义。但是我们可以用双引号选择`GDB`里的含义。

注意！当处于刚刚进入后或者刚刚退出前，一个本地变量可能会有错误的值，这是因为大部分的架构，设置一个栈帧需要多于一个机器指令。如果是以机器指令步进的话，直到一个栈帧彻底构建完成前，变量可能会有错误的值。同理，退出一个栈帧也需要多于一个机器指令，直到一个栈帧被彻底销毁前，变量也可能有错误的值。

如果我们使用`@entry`在函数实参变量前，我们就会得到这个变量在函数刚刚开始运行时的值。

```text
Breakpoint 1, d (i=30) at gdb.base/entry-value.c:29
29  i++;
(gdb) next
30  e (i);
(gdb) print i
$1 = 31
(gdb) print i@entry
$2 = 30
```

### 数组

使用指针打印数组时，我们可以在后面加上`@len`指定打印的数组元素数量。

```CPP
int *array = (int *) malloc (len * sizeof (int));
```

在`GDB`中，我们可以

```text
p *array@len
```

### 显示内存值

可以使用命令`x`或`examine`来显示内存值。

* `x/nfu addr`

  显示指定地址的内存值，`n`,`f`,`u`控制显示内存的数量与格式。

  `n`是显示内存的数量，默认值是`1`,如果是负数，则从指定的地址逆序显示。

  `f`显示的格式，可以是`‘x’, ‘d’, ‘u’, ‘o’, ‘t’, ‘a’, ‘c’, ‘f’, ‘s’`,机器指令`i`，内存标签`m`,默认是`x`,十六进制。

  `u`是内存单元的大小，可以是字节`b`，两个字节`h`,四个字节`w`,八个字节`g`。

### 自动显示

如果想要每次程序停止时就会显示相同的表达式，可以把这个表达式加入到自动显示列表中，这个表达式会被附上一个编号。

```text
2: foo = 38
3: bar[5] = (struct hack *) 0x3804
```

自动显示表达式编号，表达式，以及表达式的值，如同使用`print`一样。

* `display expr`

  把表达式加入到自定显示列表中。

* `undisplay dnum`,`delete display dnum`

  把指定表达式从表达式列表中删除。

* `disable display dnums`,`enable display dnums`

  失能或者使能指定的表达式

* `display`

  显示当前所有自动显示的表达式的值。

* `info display`

  打印自动显示列表。

### 便捷变量

`gdb`提供了便捷变量用来保存一个值，方便后续使用，这些便捷变量只存在与`gdb`中，对程序的运行没有影响。

便捷变量都以`$`开头。

比如，我们可以设置一个变量用于保存指定的表达式的值。

```gdb
set $foo = *object_ptr
```

* `show convenience`

  显示至今为止用过的便捷变量。

* `init-if-undefined $variable = expression`

  初始化一个便捷变量。

通常使用便捷变量的情况就是连续检测数组的值时。

```gdb
set $i = 0
print bar[$i++]->contents
```

这样我们就可以使用回车键连续检测数组的值。

在使用`print`打印变量时，`gdb`就会自动赋值一个便捷变量，之后便可以引用。

```gdb
(gdb) print s
$2 = "abcd"
```

### 优化打印

`gdb`提供了一个使用`python`代码更好地打印数据结构的机制，它大幅简化了复杂类的显示。

当`gdb`打印一个变量时，它会首先寻找这个类型是否有优化打印的方法注册，如果有，`gdb`就会调用优化打印来打印这个变量。反之，则普通地打印。

优化打印通常由名字，可以简单地操纵。使用命令`info pretty-printer`就会列出所有已安装的优化打印方法与它们的名字。如果一个优化打印可以处理多种数据类型，它就会有一个子优化打印用于每个独立的数据类型，子优化打印也有名字，形如`printer-name;subprinter-name`.

通常优化打印方法会在调试信息加载时自动注册。

优化打印依照作用域分为三种，

* 全局可用的优化方法
* 特定程序可用的优化方法
* 特定文件（比如共享库）可用的优化方法。

比如，`std::string`在没有优化打印时显示如下

```gdb
(gdb) print s
$1 = {
  static npos = 4294967295, 
  _M_dataplus = {
    <std::allocator<char>> = {
      <__gnu_cxx::new_allocator<char>> = {
        <No data fields>}, <No data fields>
      },
    members of std::basic_string<char, std::char_traits<char>,
      std::allocator<char> >::_Alloc_hider:
    _M_p = 0x804a014 "abcd"
  }
}
```

而在有优化打印时如下

```gdb
(gdb) print s
$2 = "abcd"
```

#### 安装优化打印

优化打印方法是使用`python`编写的，通常大型库都会提供优化打印方法的文件，我们只需要安装即可。

进入用户根目录`~`,创建（或修改）`.gdbinit`文件，文件如下

```python
python
import sys
sys.path.insert(0, '/cygdrive/d/gdb_pretty/Eigen')
from printers import register_eigen_printers
register_eigen_printers(None)

sys.path.insert(0, '/usr/share/gcc-11/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers(None)
```

其中`sys.path.insert(0, '/cygdrive/d/gdb_pretty/Eigen')`是把优化打印的文件添加到`path`环境变量中。`from printers import register_eigen_printers`，就是从`printers.py`文件中引入`register_eigen_printers`函数。

这个文件会在`gdb`启动时自动读取，例子注册了`Eigen`矩阵库的优化打印和`stl`容器的优化打印。

## 调试多`inferior`程序

`GDB`允许用户在单个`GDB`中运行并调试多个程序。此外，某些系统上的`GDB`允许同时运行多个程序（否则必须先退出一个程序，然后才能启动另一个程序）.

每个`inferior`都有一个`id`，这个`id`与进程`id`不同。通常每个`inferior`还具有自己独有的地址空间。此外，每个`inferior`可能还有多个线程在其中运行。

* `info inferiors`

  打印当前由`gdb`管理的所有`inferiors`的信息，默认打印所有`inferior`，但是也可以指定`id`号。

  `GDB`显示的信息如下

  1. `inferior`的`id`号
  2. 目标系统的`inferior`标识符，比如Linux下的进程id.
  3. `inferior`连接到`GDB`的方法，比如是本地连接或者是网络连接。
  4. `inferior`正在运行的可执行文件的名称。

  `*`号表示`GDB`当前的`inferior`

  ```shell
  (gdb) info inferiors
    Num  Description       Connection                      Executable
  * 1    process 3401      1 (native)                      goodbye
    2    process 2307      2 (extended-remote host:10000)  hello
  ```

* `inferior`

  显示当前`inferior`的信息

* `add-inferior [ -copies n ] [ -exec executable ] [-no-connection ]`

  添加`n`个`inferior`,它们都运行可执行文件`executable`,`n`默认为`1`.如果没有指定任何可执行文件，`inferior`就是空的。之后仍然可以使用`file`命令指定可执行文件。

* `clone-inferior [ -copies n ] [ infno ]`

  添加`n`个`inferior`,它们运行与`infno`相同的程序。

* `remove-inferiors infno…`

  移去`infno`指定的`inferior`,无法移去正在运行的`inferior`，需要首先`kill`或`detach`.

* `detach inferior infno…`

  把`infno`指定的`inferior`从`GDB`中脱离开来，让它独立运行，注意，这个`inferior`还保留在`info inferiors`中，但是它的描述变为`<null>`

* `kill inferiors infno…`

  `kill inferior`,注意，这个`inferior`还保留在`info inferiors`中，但是它的描述变为`<null>`

## 调试多线程的程序

`GDB`提供了这些用于调试多线程程序的工具,如下

* 当新线程创建时自动通知。
* `thread thread-id`切换线程
* `info threads`显示线程的信息
* `thread apply [thread-id-list | all] args`将命令应用于线程列表的命令
* 线程特定的断点
* `set print thread-events`设置当线程启动或者是退出时，打印的信息
* `set libthread-db-search-path path`让用户设置要使用的`libthread_db`.

`GDB`允许用户在程序运行时观测所有的线程，但是，`GDB`假设用户总是关注一个特定的线程，就是当前线程(current thread).`GDB`以当前线程的角度显示信息。

每当 GDB 在程序中检测到新线程时，它都会显示该线程的目标系统标识，`[New systag]`,`systag`是特定系统的线程标识符。比如

```shell
[New Thread 0x41e02940 (LWP 25582)]
```

处于`debug`的方便，`GDB`会给`inferior`的每个线程一个线程号，用于标识相同`inferior`下线程.但是在不同的`inferior`中，线程号可能重复。

可以使用`inferior-num.thread-num`来指定`inferior-num`下的`thread-num`线程，如果省略`inferior-num`则指的是当前`inferior`.

一些命令可以接受一系列线程，比如`info threads`,指定一系列线程的方法如下

1. 使用线程号指明一个特定的线程
2. 使用`-`指定一个范围的线程，`1.2-4`
3. 使用`*`指定所有的线程，`1.*`.

* `info threads [-gid] [thread-id-list]`

  显示指定线程的信息，默认是全部线程。

  `GDB`给每个线程显示的信息如下

  1. `GDB`分配给线程的线程号，如果没有多`inferior`，也不会显示`inferior`号。
  2. `GDB`全局的线程号，如果没有多多`inferior`，也不会显示全局的线程号。
  3. 目标系统的线程标识号
  4. 线程名，线程可以由用户命名，或者在某些情况下由程序本身命名。
  5. 每个线程当前栈帧摘要

  同时还会有`*`表示当前线程。

  ```gdb
  (gdb) info threads
    Id   Target Id             Frame
  * 1    process 35 thread 13  main (argc=1, argv=0x7ffffff8)
    2    process 35 thread 23  0x34e5 in sigpause ()
    3    process 35 thread 27  0x34e5 in sigpause ()
      at threadtest.c:68
  ```

* `thread thread-id`

  切换当前线程为`thread-id`.

  `GDB`使用`thread-id`的标识号与当前栈帧回复

  ```gdb
  (gdb) thread 2
  [Switching to thread 2 (Thread 0xb7fdab70 (LWP 12747))]
  #0  some_function (ignore=0x0) at example.c:8
  8     printf ("hello\n");
  ```

## 指定调试目标

通常，目标通过`file`，`core`命令的自动指明，但是，当使用`GDB`远程调试，或者是通过`TCP/IP`调试一个嵌入式设备，就需要指明目标。

### 远程调试类型

`GDB`支持两种远程调试类型，`target remote`模式和`target extended-remote`模式，通常后者比前者要强大，比如`target remote`模式下，`GDB`不支持`run`命令，但`target extended-remote`支持。但是许多远程目标只能使用`target remote`模式.

通常使用情况是使用`arm-none-eabi-gdb`调试单片机上的嵌入式程序.

* `target remote serial-device`
* `target extended-remote serial-device`
  
  通过串口连接远程程序到`GDB`.比如

  ```gdb
  target remote /dev/ttyb
  ```

* `target remote local-socket`
* `target extended-remote local-socket`

  通过本地套接字连接远程程序到`GDB`.

* `target remote host:port`
* `target remote [host]:port`
* `target remote tcp:host:port`
* `target remote tcp:[host]:port`
* `target remote tcp4:host:port`
* `target remote tcp6:host:port`
* `target remote tcp6:[host]:port`
* `target extended-remote host:port`
* `target extended-remote [host]:port`
* `target extended-remote tcp:host:port`
* `target extended-remote tcp:[host]:port`
* `target extended-remote tcp4:host:port`
* `target extended-remote tcp6:host:port`
* `target extended-remote tcp6:[host]:port`

  通过`TCP`连接远程程序到`GDB`.

* `target remote udp:host:port`
* `target remote udp:[host]:port`
* `target remote udp4:host:port`
* `target remote udp6:[host]:port`
* `target extended-remote udp:host:port`
* `target extended-remote udp:host:port`
* `target extended-remote udp:[host]:port`
* `target extended-remote udp4:host:port`
* `target extended-remote udp6:host:port`
* `target extended-remote udp6:[host]:port`

  通过`UDP`连接远程程序到`GDB`.
