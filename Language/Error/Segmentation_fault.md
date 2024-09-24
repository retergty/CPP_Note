# Segmentation fault

段错误(Segmentation fault)是一个特定的应用程序错误,通常是由于访问(acess)了不属于当前应用程序的内存导致的.

参考文档

* [What is a segmentation fault](https://stackoverflow.com/questions/2346806/what-is-a-segmentation-fault)

## 可能产生原因

### 解引用指向非法位置的指针

最常见的就是解引用空指针.

```CPP
int *p = NULL;
*p = 1;
```

### 向不可写内存区域写，执行不可执行的内存区域

```CPP
char *str = "Foo"; // Compiler marks the constant string as read-only
*str = 'b'; // Which means this is illegal and results in a segfault
```

### 悬垂指针，悬垂引用

```CPP
char *p = NULL;
{
    char c;
    p = &c;
}
// Now p is dangling
```

由于栈空间变量具有自动生命周期,超出生命周期后，指针变为悬垂指针.

悬垂指针可能会与函数调用，结构体结合，从而带来奇怪的结果.



## 调试方法

