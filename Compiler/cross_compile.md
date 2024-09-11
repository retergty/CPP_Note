# cross compile

本文总结`C/C++`跨平台编译方法.

## 安装跨平台编译器

假设目标架构为`ARM`，首先需要安装跨平台编译器，安装目录在[Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

要选择的编译器取决于当前系统架构与目标系统架构，以及程序是否是裸机程序.对于运行在裸机