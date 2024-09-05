# compiler tools

除了常见的`gcc`,`g++`工具，编译器还提供了一系列的工具，可以对编译出来的对象进行操作，比如删除特定内容，把文件格式转换为其它格式等。

注意，对于非交叉编译环境，这些工具就是下面的名字；但对于交叉编译环境，这些工具需要添加交叉编译工具前缀，比如`arm-none-eabi-`

## 概念

### 符号表

符号表是编译时产生的一个`hash`列表，一般包含在可执行文件里，符号表包括了变量和函数的信息，以及调试信息，可以通过`nm`工具查看符号表。符号表主要由编译器编译时，链接器链接动态库时，调试器调试时使用.对于可执行文件，去除符号表只会影响它的调试时的难度，但是可以显著降低程序大小。

## strip

```shell
strip <option(s)> in-file(s)
```

用于移除文件中的符号表。

* `-s,--strip-all`最常用的选项，去除文件中的所有符号表与调试信息,用于最小化文件大小并移除所有符号表和调试信息。
* `--strip-debug`仅去除调试信息，保留符号表。使用此选项可以减小文件大小，但仍保留符号表以便进行符号级别的调试.
* `--strip-unneeded`去除未使用的符号表。此选项会移除未被动态库使用的符号，减小文件的大小。
* `--only-keep-debug`仅保留调试信息，移除所有其他内容。使用此选项可以将符号表和其他非调试信息全部移除，只保留调试信息。
* `-o <file>`把修改后的文件保存在`file`中，而不是就地修改。

## nm

```shell
nm [option(s)] [file(s)]
```

打印符号表以及相关的信息。

* `-S,--print-size`打印符号大小，比如函数所占的空间，全局变量所占的空间。
* `--size-sort`按照符号的大小排序，从小到大。
* `-t radix`,`--radix=radix`按照`radix`指定的进制打印值，`d`为十进制，`o`为八进制，`x`为十六进制。

对每个符号表，`nm`会打印符号值，符号类型，符号名。

符号值指的就是符号所在的内存地址，比如函数所在的内存地址。

符号类型指的是符号具体是什么内容，可以是如下的值

* `B`,`b`符号是未初始化的数据段，`.bss`段.
* `D`,`d`符号是已初始化的数据段，`.data`段.
* `T`,`t`符号是代码段，`data`段。
* `W`,`w`符号是弱符号,也就是使用了`weak`修饰的符号。

### 打印符号大小并排序

```shell
arm-none-eabi-nm --print-size --size-sort --radix=d Micro-XRCE-DDS-Client-MCU.elf
```

打印了符号大小并排序，用于进行优化减少代码量。

## objcopy

```shell
objcopy [option(s)] in-file [out-file]
```

复制一个文件，同时按照选项指定的方法改变文件，比如转换格式，删除某些内容等。

* `-O --output-target <bfdname>`把输出文件转换为`<bfdname>`格式。

`<bfdname>`格式通常有`ihex`用于串口下载的`hex`格式，`binary`纯粹的二进制文件，几乎就是下载到单片机里程序的最终大小，除了`8`个字节的文件标头。

### 把可执行文件转化为hex文件用于串口下载

```shell
arm-none-eabi-objcopy -O ihex Micro-XRCE-DDS-Client-MCU.elf Micro-XRCE-DDS-Client-MCU.bin
```

### 把可执行文件转化为bin文件查看最终大小

```shell
arm-none-eabi-objcopy -O binary Micro-XRCE-DDS-Client-MCU.elf Micro-XRCE-DDS-Client-MCU.bin
```

## size

```shell
size [option(s)] [file(s)]
```

打印文件各部分大小或者是段大小等信息。

* `--format=berkeley`按照`berkeley`格式打印,默认格式
* `--format=SysV`按照`SysV`格式打印。
* `--radix=<num>`按照`num`进制打印，默认是十进制。

### berkeley格式

`berkeley`格式例子如下

```shell
size Micro-XRCE-DDS-Client-MCU.elf
```

输出如下

```text
 text     data      bss      dec      hex  filename
69024      232    25960    95216    173f0  Micro-XRCE-DDS-Client-MCU.elf
```

`text`是代码段，`data`是数据段，`bss`是未初始化的数据段。只有这三段与程序最终运行有关。

### SysV格式

`SysV`格式如下

```shell
arm-none-eabi-size --format=SysV --radix=16 Micro-XRCE-DDS-Client-MCU.elf
```

```text
section                size         addr
.isr_vector           0x188    0x8000000
.text               0x10508    0x8000190
.rodata               0x708    0x8010698
.ARM                    0x8    0x8010da0
.init_array             0x4    0x8010da8
.fini_array             0x4    0x8010dac
.data                  0xe0   0x20000000
.ccmram                 0x0   0x10000000
.bss                 0x5f68   0x200000e0
._user_heap_stack     0x600   0x20006048
.ARM.attributes        0x30          0x0
.comment               0x45          0x0
.debug_info         0x18824          0x0
.debug_abbrev        0x2bac          0x0
.debug_aranges       0x1398          0x0
.debug_rnglists       0xee7          0x0
.debug_line          0xebf4          0x0
.debug_str           0x6a53          0x0
.debug_frame         0x5fc8          0x0
.debug_line_str       0x1cd          0x0
Total               0x50290
```

打印了所有段大小以及内存位置，有的段与`debug`有关，没有实际的内存地址。
