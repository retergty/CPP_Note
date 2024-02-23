# 常用编译器选项总结

本文总结了常用的编译器选项

参考文章

* [gcc option summary](https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html)

## 在代码中插入额外指令的选项

参考文章

* [gcc Program Instrumentation Options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)

### `--coverage`

这个选项用于C++代码覆盖测试。这个选项是`-fprofile-arcs`，`-ftest-coverage`（编译时）,`-lgcov`（链接时）的别名。

### `-fprofile-arcs`

自动添加用于测试程序流程的指令，记录在处理程序时，每个分支和调用的执行次数。当程序终止时，将记录的数据写到`auxname.gcda`里，每个源文件都有一个对应的文件，比如对于`dir/foo.c`,`foo.gcda`。

如果命令直接链接了源文件，那么生成的`.gcda`还会以可执行文件的文件名命名。比如`gcc a.c b.c -o binary`会生成`binary-a.gcda`和`binary-b.gcda`。

### `-ftest-coverage`

对每个源文件，产生一个可用于`gcov`代码覆盖程序的注释文件，命名为`auxname.gcno`.

### `-lgcov`

严格来讲，这不是`gcc`选项，而是链接`gcov`库。

## 提供或者抑制警告的选项

参考文章

* [Options to Request or Suppress Warnings](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)

### `-Wpedantic`,`-pedantic`

产生所有`ISO C`与`ISO C++`要求的警告。

### `-Wall`

使能**大量的警告**，这些警告通常都是错误地编写代码导致的，并且可以简单地避免。

### `-Wextra`

使能大量的没有被`-wall`使能的**额外警告**。

### `-Warray-bounds`,`-Warray-bounds=n`

警告超**出界限数组访问**，`n`通常为`1`。

### `-Wcast-align`

警告当指针转换时，要求的**字节对齐提高**且对应的目标机器**不支持非对齐访问**，比如警告`char *`强制转换为`int *`.

### `-Wcast-qual`

警告当强制类型转换出去了类型限定符，比如从`char*`类型转换为`const char*`.

也会警告类型转换引入了一个不安全的类型限定符。比如二重指针，把`char **`转换为`const char **`.

```CPP
/* p is char ** value.  */
const char **q = (const char **) p;
/* Assignment of readonly string to const char * is OK.  */
*q = "string";
/* Now char** pointer points to read-only memory.  */
**p = 'b';
```

正如上述的代码块，由于`*q`类型为`const char*`所以可以指向只读存储区，但是由于`*p`的类型为`char *`指向只读存储区显然是错误的。

### `-Wconversion`

警告隐式类型转换可能会丢失数值，比如`unsigned ui = -1`.对于`C++`还会警告用户定义的类型转换的问题。

### `-Wctor-dtor-privacy`

`C++`专有，警告一个类似乎不可使用，因为所有的构造和析构函数都是私有成员，而且没有友元或者是公有静态成员函数。也会警告一个类没有非私有方法且至少有一个不是构造或者是析构函数的私有成员函数。

### `-Wdisabled-optimization`

警告当一个要求的优化过程被禁用。这个警告通常不表明错误，只是表明`GCC`优化器不能有效地处理这个代码。

### `-Werror`

把所有警告认为是错误。

### `-Wfloat-equal`

警告当浮点数被用于相等性判断中。

### `-Wformat-security`

如果声明了`-Wformat`，也警告当使用格式化函数时可能的安全问题。目前，这个警告有关于`printf`与`scanf`函数有关。

### `-Wformat`,`-Wformat=n`

检查所有的`printf`，`scanf`等格式化函数的调用，保证实参与格式化字符串指定的参数一致。`n`表示检查的层级，默认是`1`;为`2`时使能额外的格式检查参数`-Wformat -Wformat-nonliteral -Wformat-security -Wformat-y2k.`.

### `-Winit-self`

包含在`-Wall`中，警告所有未初始化的变量初始化为其本身，必须与`-Wuninitialized`同用。