# Linker Scripts

`Linker Scripts`是一个`.ld`文件，控制了链接器进行链接的整个过程。

参考文档

* [GNU 官方文档Command Language](https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html#SEC5)

## object文件预定义的输入段

哪怕没有在代码中定义输入段，编译器也会创建预定义的输入段，并把相对应的符号放在输入段中.

* `.text`,`.text*`段用于放置可执行的代码.
* `.data`,`.data*`段用于放置已初始化的静态变量，比如`int a = 10;`.
* `.rodata`,`.rodata*`段用于放置只读变量，比如`const string str = "Hello World\r\n";`.
* `.bss`，`.bss*`段用于放置未初始化的静态变量，比如`int a[255];`.

用户可以使用`gcc/g++`扩展来指定对象所在的输入段

```CPP
__attribute__((section("section_name")))
```

```CPP
void foo() __attribute__((section(".text_foo")));
....
void foo() {}
```

## 表达式

`Linker Scripts`中表达式的语法与C表达式的语法相同，具有以下特点

* 所有表达式均计算为整数，且均为`long`或`unsigned long`类型。
* 所有常量都是整数.
* 提供了所有C风格算术运算符.
* 可以引用、定义和创建全局变量.
* 可以调用特殊用途的内置函数.

### 整数

```linkerscript
_as_octal = 0157255; /* 八进制整数 */
_as_decimal = 57005; /* 十进制整数 */
_as_hex = 0xdead; /* 十六进制整数 */
_as_neg = -57005; /* 十进制负整数 */
```

还可以使用`K`,`M`后缀.

```linkerscript
_fourk_1 = 4K;
_fourk_2 = 4096;
_fourk_3 = 0x1000;
```

### 符号名称

除非加引号，否则符号名称以字母、下划线或点开头，并且可以包含任何字母、下划线、数字、点和连字符。不加引号的符号名称不能包含空格,且不能与任何关键字冲突.

```linkerscript
"SECTION" = 9;
"with a space" = "also with a space" + 10;
```

### 地址计数器

地址计数器`Location Counter`是链接文件预定义的一个特殊的变量，它是一个`.`,包含了当前输出位置的地址值。它必须出现在`SECTIONS`命令里，地址计数器可以出现在任何合法的表达式位置，但是给它赋值具有副作用。为`.`赋值将导致地址计数器移动，从而在输出位置创建空洞，地址计数器永远不会向后移动。

```linkerscript
SECTIONS
{
  output :
  {
  file1(.text)
  . = . + 1000;
  file2(.text)
  . += 1000;
  file3(.text)
  } = 0x1234;
}
```

上述的例子中，`file1`中的`.text`段被放置在`output`节的开始位置，之后存在`1000`个字节的空洞，随后放置`file2`中的`.text`段，以此类推。`= 0x1234`描述了空洞应该存放的值。

### 表达式求值

链接器使用表达式延迟求值，只在必须求值的上下文中对表达式进行求值。链接器需要知道起始地址的值和内存区域的长度，以便进行任何链接；所以，当链接器读入链接文件时，会尽快计算这些值。然而，其他值（例如符号值）直到存储分配之后才知道或需要。

### 定义符号

可以创建全局符号，全局符号可以给源文件使用，并给它附上相应的地址值

```linkerscript
symbol = expression ;
symbol &= expression ;
symbol += expression ;
symbol -= expression ;
symbol *= expression ;
symbol /= expression ;
```

注意，定义符号后面的`;`是必须的。

当创建变量时，链接器会给它绝对或者是相对类型，绝对类型指的就是符号原样值，相对类型就是符号被表达为相对于`SECTIONS`基地址的偏移。

变量的类型取决于它定义的位置，在一个`section`中定义的变量就是相对的，其余都是绝对的，但是可以使用`ABSOLUTE`函数.

```linkerscript
SECTIONS{ ...
  .data : 
    {
      *(.data)
      _edata = ABSOLUTE(.) ;
    } 
... }
```

### 算术函数

链接文件包含许多内置函数.

* `ABSOLUTE(exp)`

  返回表达式`exp`的绝对值.

* `ADDR(section)`

  返回段`section`的绝对地址，链接脚本必须已定义该段的位置。

  ```linkerscript
  SECTIONS{ ...
    .output1 :
      { 
      start_of_output_1 = ABSOLUTE(.);
      ...
      }
    .output :
      {
      symbol_1 = ADDR(.output1);
      symbol_2 = start_of_output_1;
      }
  ... }
  ```

  `symbol_1`和`symbol_2`的值相同。

* `LOADADDR(section)`

  返回段`section`的绝对加载地址，通常和`ADDR(section)`一致，除非使用了`AT`指定了不同的加载地址，比如把初始化的数据加载到`FLASH`,但是运行时是在`RAM`上.

* `ALIGN(exp)`

  返回与当前地址计数器与`exp`对齐的值。和`(. + exp - 1) & ~(exp - 1)`相同.

## 内存布局

链接器的默认配置允许分配所有可用内存。可以使`MEMORY`命令覆盖此配置.`MEMORY`命令描述目标中内存块的位置和大小。描述了链接器可以使用哪些内存区域，以及必须避免哪些内存区域。链接器不会打乱段以适合可用区域，但会将请求的部分移动到正确的区域，并在区域超出可用时发出错误。

`MEMORY`格式如下

```linkerscript
MEMORY 
  {
    name (attr) : ORIGIN = origin, LENGTH = len
    ...
  }
```

* `name`该内存区域的名字，链接脚本的其它位置可以使用这个名字指定该内存区域.内存区域名称存储在单独的名称空间中，不会与符号、文件名或段名冲突。
* `(attr)`描述了该内存区域的功能，支持的标签如下
  * `R`只读内存区域
  * `W`可读可写内存区域
  * `X`包含可执行代码的内存区域
  * `A`已分配的内存区域
  * `I`已初始化的内存区域
  * `!`反转标签的含义
* `origin`这个内存区域在实际物理内存的起始地址.
* `len`这个内存区域的长度，以字节计.

```linkerscript
MEMORY 
  {
  rom (rx)  : ORIGIN = 0, LENGTH = 256K
  ram (!rx) : org = 0x40000000, l = 4M
  }
```

上述定义了两个内存区域，一个是`rom`用来存储代码，数据段，另一个是`ram`用来存储可变数据。

## 输出段

`SECTIONS`命令直接控制输入段要放置在的输出段,放置的顺序等，输入段就是`.bss`段，`.data`段等,输出段由链接脚本定义.

大多数链接脚本只会使用一次`SECTIONS`命令，但是`SECTIONS`命令可以多次使用。`SECTIONS`命令中，有大致三种操作

* 定义入口点(entry point)
* 定义符号并赋值
* 描述一个输出段并在其中放置输入段

### 定义段

最常使用的就是定义段，声明了输出段的属性,比如位置，对齐，内容，模式与目标内存区域.

```linkerscript
SECTIONS { ...
  secname : {
    contents
  }
... }
```

`secname`是输出段的名称，不会与输入段的名称冲突，`contents`包含输入段，注意`:`左边的空格是必须的,为了防止歧义.

一个特殊的输出段`/DISCARD/`用于抛弃其中的输入段,任何在其中的输入段都不会包含在最终的输出文件中。

如果一个输出段不包含任何`contents`,链接器不会实际创建这个输出段，比如

```linkerscript
.foo { *(.foo) }
```

只会在`.foo`实际有输入段时才会创建`.foo`输出段.

### 放置输入段

`contents`包含输入段，可以通过直接指定文件，直接指定文件里的段等等方法.

`contents`支持格式如下

* `filename`

  直接指定`filename`文件，该文件中所有的输入段都会被放置在这个输出段中.如果`filename`已经在之前的另一个输出段中指定，且不是通配符，那么只会在当前输出段放置之前尚未被分配的输入段。

  ```linkerscript
  .data : { afile.o bfile.o cfile.o }
  ```

* `filename( section )`
* `filename( section , section, ... )`
* `filename( section section ... )`

  把`filename`文件中的对应输入段`section`放置在当前输出段中.

* `* (section)`
* `(section, section, ...)`
* `(section section ...)`

  把所有输入文件中的`section`段放置在这个输出段中，直接指定`filename`文件的优先级高于这个.也就是说`*`指的是所有剩余的文件。

* `filename( COMMON )`
* `*( COMMON )`

  `COMMON`指的是所有的未初始化的变量。链接器允许通过`COMMON`指定所有未初始化的数据，就好像它们都是在输入段`COMMON`中的.

指定文件名时可以使用通配符，但是链接脚本里的通配符不会匹配`/`，因为这个是`Unix`里的目录分隔符。单独使用的`*`是一个例外.

指定`section`时也可以使用通配符，不同于文件名，它会匹配`/`.

```linkerscript
SECTIONS { 
  .text : { *(.text) }
  .data : { *(.data) } 
  .bss :  { *(.bss)  *(COMMON) } 
} 
```

### 可选输出段属性

输出段完全定义如下

```linkerscript
SECTIONS {
...
secname start BLOCK(align) (NOLOAD) : AT ( ldadr )
  { contents } >region :phdr =fill
...
}
```

* `start`

  强制指定输出段加载到的内存地址，可以是任何表达式.

  ```linkerscript
  SECTIONS {
    ...
    output 0x40000000: {
      ...
      }
    ...
  }
  ```

* `BLOCK(align)`

  增加地址计数器，达到`align`指定的对齐内存.

* `AT ( ldadr )`，`AT>region`

  指定输出段的加载地址，和执行地址不同，在C语言运行前的Startup文件会把输出段从加载地址复制到执行地址，之后代码在执行地址允许。`ldadr`表示具体地址，`region`表示内存区域.

* `>region`

  指定输出段的执行地址，`region`表示内存区域.
