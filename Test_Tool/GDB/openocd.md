# openocd

`openocd`是一个用于嵌入式`debug`的工具，支持常用的协议，比如`JTAG`,`SWD`.

参考文档

* [OpenOCD User’s Guide](https://openocd.org/doc/html/index.html)

## 启动流程

`openocd`首先处理通过命令行传入的配置文件或者是命令，之后便会启动`openocd`服务，随后使用`gdb`监听端口便可以进行调试.

### 配置阶段

当`openocd`启动时，它便进入了配置阶段。在配置阶段，只有一些特定的命令可以运行。

### 运行阶段

离开配置阶段后，`openocd`便进入了运行阶段。

## 命令行参数

* `--file,-f`指定要读取的配置文件，可以读取多个配置文件，只需要重复使用`-f`即可.
* `--command,-c`运行命令.

包含配置文件的文件目录通常是`/usr/share/openocd/scripts`.

## 常见命令

* `exit`
  退出.

* `reset`
  进行硬件`reset`

* `reset run`

  进行`reset`，并让硬件自由运行

* `reset halt`

  进行`reset`，并立刻停止硬件

* `reset init`

  进行`reset`,并停止硬件，同时执行任何`reset init`的脚本

* `init`

  终止配置阶段，进入运行阶段.

* `program filename [verify] [reset] [exit] [preverify]`

  把`filename`指定的`elf`文件复制进`flash`.同时按顺序进行

  1. 执行`init`命令
  2. 执行`reset init`命令
  3. 执行`write_image`命令，擦除并写入指定的文件
  4. 如果使用了`preverify`参数，那么会先进行`verify`，如果出错才会`flash`.
  5. 如果使用了`verify`参数，会执行`verify_image`.
  6. 如果使用了`reset`参数，那么会执行`reset run`命令.
  7. 如果使用了`exit`参数，那么会执行`exit`命令.

* `transport select [transport_name]`

  选择特定的传输协议，比如`jtag`,`swd`,`spi`等。

## 例子

```shell
openocd -f interface/jlink.cfg -c "transport select swd"  -f target/stm32f4x.cfg  -c "program ./Micro-XRCE-DDS-Client-MCU.elf verify"
```
