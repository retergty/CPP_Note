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

由于段错误发生在访问内存时而不是给指针赋非法值时，所以产生的错误位置会比较奇怪。

使用`gdb`，配合`address santizier`时，会在产生段错误时产生断点.

以下归纳产生原因会发生的典型情况.

### 解引用指向非法位置的指针

此时程序会停留在解引用指针时

```CPP
*p;
p->a;
p->sub_p->a;
```

使用`gdb`打印当前指针的值，就会发现指针的错误，注意多级指针每个指针的值都需要考虑。

### 向不可写内存区域写，执行不可执行的内存区域

向不可写内存区域写，程序会停留在写访问时.

```CPP
char *str = "Foo"; // Compiler marks the constant string as read-only
*str = 'b'; // Which means this is illegal and results in a segfault
```

使用`gdb`查看指针的值，配合内存地址的属性即可发现错误.

执行不可执行的内存区域，通常是栈溢出的结果，程序会停留在第一个非法执行地址。

使用`gdb`查看`pc`指针的值，配合内存地址的属性即可发现错误.

### 悬垂指针，悬垂引用

由于开启了`address sanitizer`，所有释放的栈空间会被附上一个特殊的值，这个值对应的内存区域不可读写，所以程序停留在解引用指针。

```CPP
char *p = NULL;
{
    char c;
    p = &c;
}
*p;
```

使用`gdb`查看`sp`指针的值，与产生错误的指针的值进行比较，即可发现`p`的值大于`sp`,指向一个已释放的区域.注意，如果是多级指针，从第一级指针开始比较.

通常发生位置固定，在作用域退出时.

```CPP
char *p = malloc(sizeof(char));
free(p);
*p;
```

由于开启了`address sanitizer`，释放后使用会停留在解引用指针处.
