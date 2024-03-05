# UndefinedBehaviorSanitizer

`UndefinedBehaviorSanitizer`(UBSan)是一个快速的未定义行为检测工具，UBSan在编译期修改程序，以检测运行时的未定义行为。比如

* 数组下标越界
* 移位超出界限
* 解引用未对齐或是Null指针
* 有符号整数溢出
* 浮点类型转换导致的数据溢出

等等

参考文档

* [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)

## 用法

指定编译选项`-fsanitize=undefined`即可使用这个工具。
