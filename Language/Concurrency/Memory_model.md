# Memory model

参考文档

* [Memory model](https://en.cppreference.com/w/cpp/language/memory_model)
* [C++11 introduced a standardized memory model. What does it mean? And how is it going to affect C++ programming?](https://stackoverflow.com/questions/6319146/c11-introduced-a-standardized-memory-model-what-does-it-mean-and-how-is-it-g)

`C++`内存模型(Memory model)定义了计算机内存存储的语义，内存模型允许编译器进行许多重要的优化，基于内存模型，编译器会重排数据的读取与写入，对于多线程程序，不理解内存模型可以会在不知晓的时候引入数据竞争。

`C++`程序可用的内存就是一个或者多个连续的字节，每个字节在内存中都有独一无二的地址。

## 字节

字节(Byte)是最小的内存单元，它由一系列的比特组成(通常为8个).

`char`,`unsigned char`,`signed char`大小为一个字节。

## 内存位置

内存位置(memory location)是

* 一个特定的对象
* 非零长度的最大连续位域序列

```CPP
struct S
{
    char a;     // memory location #1
    int b : 5;  // memory location #2
    int c : 11, // memory location #2 (continued)
          : 0,
        d : 8;  // memory location #3
    struct
    {
        int ee : 8; // memory location #4
    } e;
} obj; // The object “obj” consists of 4 separate memory locations
```
