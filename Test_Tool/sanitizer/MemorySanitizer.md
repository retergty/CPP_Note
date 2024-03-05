# MemorySanitizer

`MemorySanitizer`是一个检测访问未初始化内存区域的检测工具。

未初始化变量来自于对于堆栈的未初始化变量的读取.`MemorySanitizer`MSan 检测此类值影响程序执行的情况.

`MemorySanitizer`是位精确的：它可以追踪位字段中的未初始化的位。它可以容忍未初始化内存的复制，以及简单的逻辑和算术运算。总的来说，`MemorySanitizer`会追踪未初始化的数据的传播，并报告如果代码访问了未初始化的数据。

参考文档

* [MemorySanitizer](https://github.com/google/sanitizers/wiki/MemorySanitizer)

## 用法

`MemorySanitizer`是`LLVM`的一部分，只可以使用`clang`编译器。

指定编译选项`-fsanitize=memory`,`-fPIE`,`-pie`就可以使用`MemorySanitizer`.

```CPP
% cat umr.cc
#include <stdio.h>

int main(int argc, char** argv) {
  int* a = new int[10];
  a[5] = 0;
  if (a[argc])
    printf("xx\n");
  return 0;
}
%clang -fsanitize=memory -fPIE -pie -fno-omit-frame-pointer -g -O2 umr.cc
% ./a.out
==6726==  WARNING: MemorySanitizer: UMR (uninitialized-memory-read)
    #0 0x7fd1c2944171 in main umr.cc:6
    #1 0x7fd1c1d4676c in __libc_start_main /build/buildd/eglibc-2.15/csu/libc-start.c:226
```
