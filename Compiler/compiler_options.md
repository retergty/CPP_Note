# 常用编译器选项总结

本文总结了常用的编译器选项

对于以`-f`和`-W`开头的选项，大部分都有正面或者是负面的版本，比如`-ffoo`时正面版本，`-fno-foo`是负面的版本。正面版本开启编译器的特定行为，负面版本则关闭编译器对应的特定行为。本文只有其中一个版本，另一个版本是默认情况。

参考文章

* [gcc option summary](https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html)

## 代码生成选项

参考文章

* [Options for Code Generation Conventions](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)

### `-fpic`

如果目标机器支持，产生位置无关代码。这种类型的代码会把所有常量存储在`GOT`表里。如果`GOT`表过大，就会产生连接错误。

### `-fPIC`

如果目标机器支持，产生位置无关代码,且去除了最大允许的`GOT`表的限制。

## 链接时选项

参考文章

* [Options for Linking](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)

### `-pie`

如果目标主机支持，生成动态链接的位置独立可执行文件。通常还需要与编译时的选项`-fpie`或`-fPIE`一起使用。

### `-llibrary`,`-l library`

在链接时搜索名为`library`的库，这个选项会直接传递给链接器。具体细节取决于链接器的实现，本文以`GNU`链接器为例。

链接器搜索一系列的库文件目录，包括一些标准的库文件目录以及使用`-Ldir`指定的目录。

静态库是一系列的`.o`文件的集合，其名称通常类似于`liblibrary.a`。某些目标还支持共享库，其名称通常类似于`liblibrary.so`。如果同时找到静态库和共享库，链接器会优先链接共享库，除非使用了`-static`选项。

### `-static`

在支持动态链接的系统上，这会覆盖`-pie`并阻止与共享库的链接。在其他系统上，此选项无效。

### `-shared`

生成一个共享库，可以与其它对象链接成可执行文件，为了获得可预测的结果，在指定此链接器选项时，还必须指定用于编译的同一组选项(`-fpic`、`-fPIC`).

```shell
gcc -shared add.o div.o mult.o sub.o -o libMyTest.so 
```

把`.o`文件打包为动态库。

### `-Wl,option`

给链接器传递参数。

比如`-Wl,-rpath=.`指定程序运行时优先查找的库路径，这个的优先级高于`LD_LIBRARY_PATH`.比如这个`.`就表示优先查找当前目录的库路径。

## 在代码中插入额外指令的选项

参考文章

* [gcc Program Instrumentation Options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)

### `-fsanitize=address`

使能一个检测内存错误的工具`AddressSanitizer`，能够检测越界，使用已经释放的内存等运行时的程序错误。

* [AddressSanitizer](../Test_Tool/sanitizer/AddressSanitizer.md)

### `-fsanitize=thread`

使能一个检测线程数据竞争的工具`ThreadSanitizer`,能够检测不同线程间的数据竞争等运行时程序错误。

* [ThreadSanitizer](../Test_Tool/sanitizer/ThreadSanitizer.md)

### `-fsanitize=memory`

使能一个检测未初始化的内存区域的工具`MemorySanitizer`,能够检测未初始化的内存区域的访问。

* [MemorySanitizer](../Test_Tool/sanitizer/MemorySanitizer.md)

### `-fsanitize=undefined`

使能一个检测未定义行为的工具`UndefinedBehaviorSanitizer`,可以检测运行时的未定义行为，

### `--coverage`

这个选项用于C++代码覆盖测试。这个选项是`-fprofile-arcs`，`-ftest-coverage`（编译时）,`-lgcov`（链接时）的别名。

### `-fprofile-arcs`

自动添加用于测试程序流程的指令，记录在处理程序时，每个分支和调用的执行次数。当程序终止时，将记录的数据写到`auxname.gcda`里，每个源文件都有一个对应的文件，比如对于`dir/foo.c`,`foo.gcda`。

如果命令直接链接了源文件，那么生成的`.gcda`还会以可执行文件的文件名命名。比如`gcc a.c b.c -o binary`会生成`binary-a.gcda`和`binary-b.gcda`。

### `-ftest-coverage`

对每个源文件，产生一个可用于`gcov`代码覆盖程序的注释文件，命名为`auxname.gcno`.

### `-lgcov`

严格来讲，这不是`gcc`选项，而是链接`gcov`库。

### `-fexceptions`

使能异常处理，`gcc`默认对于`C++`是开启的，但是会带来巨大的性能损失，所以建议加上`-fno-exceptions`.

## 提供编译器优化的选项

这一类的选项控制不同种类的优化行为。

如果没有指定任何优化，编译器的目标就是尽可能缩短编译时间并使得debug产生正确的结果。

如果打开优化选项，编译器就会尝试对代码进行优化，提高运行速度。

编译器运行优化的能力取决于它能得到的信息，同时编译多个源文件去产生一个输出允许编译器得到比分别编译这些源文件更多的信息。

### `-O`

### `-O1`

第一级的优化，编译器尝试降低代码大小于运行时间，而且不会提高太多的编译时间，`-O`打开了一系列的优化选项。

### `-O2`

第二级的优化，编译器尝试几乎所有支持的不涉及空间速度权衡的优化。进一步提高了编译时间。`-O2`会打开`-O1`的优化选项，而且还会打开更多的优化选项。

### `-O3`

第三级优化，编译器打开几乎所有的编译选项，进一步提高了运行速度与编译时间。

### `-O0`

降低编译时间，使得`debug`产生正确的结果，这是默认的优化选项。

### `-Os`

降低可执行文件大小。

### `-Ofast`

无视严格的标准合规性，使能所有的`-O3`优化，还会使能那么可能不是对于任何符合标准的程序都是正确的优化。

### `-finline-functions`

考虑所有函数的内联，哪怕是没有声明为`inline`的函数，让编译器自主决定什么函数更值得内联。在-O2,-O3,-Os被使能。

### `-finline-small-functions`

内联那些小函数。让编译器自主决定什么函数值得内联。在-O2,-O3,-Os使能。

### `-fno-elide-constructors`

C++标准允许省略创造一个临时变量，只用于初始化同一类型的另一个对象，这个选项失能了这个行为。

对于C++17，标准要求省略，所以这个选项不应该继续使用了。

### `-fno-rtti`

关闭`RTTI`.

### `-fno-exceptions`

关闭异常.

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

### `-Wlogical-op`

警告有关逻辑运算符可疑的使用与使用同一种逻辑运算符。

```CPP
extern int a;
if (a < 0 && a < 0) { … }
```

### `-Wmissing-declarations`

警告在使用全局函数前未声明该函数的情况。但是不会对函数模板，内联函数，或者是匿名名称空间的函数产生警告。

### `-Wmissing-include-dirs`

警告如果用户提供给编译器的`include`目录不存在。

### `-Wno-sign-compare`

关闭有关于`sign`与`unsign`数值比较的警告。

### `-Wunused`

包含在`-Wall`,警告有未使用资源的情况。

### `-Wnoexcept`

警告当`noexcept`表达式返回`false`但是编译器却认为这个函数调用不会产生异常。

### `-Wold-style-cast`

警告当`C++`程序使用了`C`风格的`cast`,比如`(int) a`.通常使用`C++`风格的类型转换,`dynamic_cast, static_cast, reinterpret_cast, and const_cast`.

### `-Woverloaded-virtual`,`-Woverloaded-virtual=n`

警告派生类的函数声明隐藏了基类的`virtual`函数声明而不是覆盖的情况。

```CPP
struct A {
  virtual void f();
};

struct B: public A {
  void f(int); // does not override
};
```

默认情况下`n=2`，当`n=2`时,要求派生类必须全部重写基类的`virtual`方法。`n=1`包含在`-Wall`中了。

```CPP
struct C {
  virtual void f();
  virtual void f(int);
};

struct D: public C {
  void f(int); // does override
}
```

### `-Wpointer-arith`

包含在`-Wpedantic`中，警告当代码中有依赖于`sizeof`函数类型或`void`.在`C++`里，也会警告在算术中使用`NULL`的情况。

### `-Wredundant-decls`

警告当一个对象在一个作用域中声明了多于一次的情况，哪怕多重声明是合法的且不改变任何东西。

### `-Wreorder`

包含在`-Wall`中，警告当在构造函数格式里成员变量初始化顺序与定义顺序不同的情况.

```CPP
struct A {
  int i;
  int j;
  A(): j (0), i (1) { }
};
```

此时。编译器会重新排序构造函数里成员变量的初始化以符合成员变量定义顺序，同时产生警告。

### `-Wshadow`

警告当本地变量或者类型声明隐藏了别的对象的情况。

### `-Wsign-conversion`

警告一个隐式转换可能会改变整型的符号，比如当使用有符号整型给无符号整型赋值时。显式的类型转换不会产生这个警告。

### `-Wsign-promo`

警告当重载决议选择了一个函数，这个函数需要把无符号提升为有符号。

### `Wstrict-null-sentinel`

警告当使用未类型转换的`NULL`作为哨兵的情况。

### `-Wswitch-default`

警告当`switch`语句不包含`default`的情况。

### `-Wundef`

警告当一个未定义的标识符出现在`#if`预处理语句中的情况，这个标识符会被替换为0.

### `-Wuninitialized`

包含在`-Wall`中，警告当一个变量初始化时。

### `-Wunused-variable`

包含在`-Wall`中，警告当一个变量未使用的情况。

### `-W-Wstrict-overflow=n`

这个选项是用于当编译器优化运算时，没有考虑到数据溢出的情况而发出的警告。通常`n=4`即可。

### `-Wno-missing-template-keyword`

如果一个类成员访问使用`.`,`->`,`::`三者之一，而且类对象是依赖对象且该成员是模板成员时，需要加上`template`.这是因为编译器可能会认为`<>`是大于和小于符号，认为成员访问是比较表达式。

```CPP
template <class X>
void DoStuff (X x)
{
  x.template DoSomeOtherStuff<X>(); // Good.
  x.DoMoreStuff<X>(); // Warning, x is dependent.
}
```

## 用于debug的选项

这一类选项告诉gcc去生成额外的信息用于debugger，在大部分情况下`-g`就足够了。

gcc允许使用`-g`加上`-O`，但是优化后的代码执行的情况可能会让debug变得复杂。

### `-g`

产生`debug`信息，通常我们可以使用`GDB`进行`debug`.

### `-glevel`

要求产生`debug`信息，信息的具体程度取决于`level`.默认的等级是`2`.

`-g0`表示不生成任何`debug`信息

`-g1`表示生成最小限度的`debug`信息

`-g3`表示生成额外的`debug`信息，比如程序中所有的宏定义。

## 传递给汇编器的选项

### `-Wa,option`

将选项`option`传递给汇编器。

常见的选项有`-mbig-obj`，允许汇编器处理大的`obj`文件。

## x86专用的选项

### `-m64`

指定编译`64`位可执行文件。
