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

