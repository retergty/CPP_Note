# cross compile

本文总结`C/C++`跨平台编译方法.

## 安装跨平台编译器

假设目标架构为`ARM`，首先需要安装跨平台编译器，安装目录在[Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

要选择的编译器取决于当前系统架构与目标系统架构，以及程序是否是裸机程序.对于运行在裸机的程序，需要安装裸机编译器，比如`arm-none-eabi-gcc`，对于运行在操作系统上的程序，需要安装带有操作系统支持的编译器，比如`arm-linux-gnueabihf-gcc`.

## 编译选项

需要给每个交叉编译器传递正确的架构相关的编译选项，才能成功编译出可在目标系统上运行的程序.

* `-mcpu`：指定目标CPU架构，比如`-mcpu=cortex-a53`。
* `-mfloat-abi`：指定浮点数的ABI，比如`-mfloat-abi=hard`。在`arm64`中，这个选项已经被废弃，默认使用`hard`。
* `-mfpu`：指定浮点单元，比如`-mfpu=neon`，在`arm64`中，这个选项已经被废弃，默认使用`neon`。
* `-system`：指定目标系统，比如`-sysroot=/path/to/sysroot`，这个选项告诉编译器在哪里找到目标系统的头文件和库文件。
