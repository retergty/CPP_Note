# makefile 学习笔记

makefile 可以实现C/C++源代码的编译，可以编写自动化的程序实现复杂的源代码编译链，它也可以自动地分析出那个文件已经更新，需要重新编译，本笔记只针对`GNU make`，因为他最常用。
本笔记详细描述了Makefile的写法，使用方法，函数等。

参考文件

* GNU 官方文档[GNU make](https://www.gnu.org/software/make/manual/make.html#Reading)

## Makefile 简介

`makefile`其实是一个文件，它指示`make`程序要执行的操作，比如编译，连接程序。
`makefile`还可以定义一系列的操作，用来运行各种各样的命令。
`makefile`的结构就相当于一个个规则的集合，不同规则间有依赖关系，各种规则结合成一个整体的`makefile`。

## Makefile 基本语法

讲解基本的Makefile的元素

参考资料

* GNU官方文档[GNU Writing Makefiles](https://www.gnu.org/software/make/manual/make.html#Makefiles)

### Makefile包含的元素种类

Makefile包含几种元素种类，Makefile就是有这些元素所组成的，它们分别是**显式规则**(explicit rule),**隐式规则**(implicit rule)，**变量定义**(variable definition),**指令**(directive),**注释**(comments),后面会详细地介绍这些元素的用法，这里粗略地描述一下。

* 显式规则(explicit rule)会告诉`make`何时以及如何重新生成一个或者多个文件，这个要重新生成的文件就是**规则目标**(rule's targets).显式规则还会声明列出规则目标所依赖文件,叫做规则目标的**先决条件**(prerequisites).显式规则一般还会声明为了生成或者更新规则目标`make`所需要执行的命令，这些命令叫做**配方**(recipe)。
* 隐式规则(implicit rule)会告诉`make`何时以及如何按照文件的名字(一般是后缀名)自动生成一类的文件。它描述了规则目标也许会依赖于与规则目标相似的文件，并且会自动生成配方创建或者生成规则目标。
* 变量定义(variable definition)是一行，它定义了**变量**，这个变量代表特定的字符串值，在`make`运行时会被替换为特定的字符串值。
* 指令(directive)是一个指令，它指示`make`做一些特定的工作，比如读取另一个Makefile，决定是否要忽略一段Makefile，定义多于一行的变量等等。
* 注释(comment)是解释说明Makefile的文字，它以`#`开始，在`make`的预处理过程就会被删除。

### 将一行分为多行

`Makefile`使用的是基于行的语法，所以换行符就标记着一个元素的结束。
但是，有时如果将所有的声明都写在一行，那么可能这一行太长了不适合人类阅读，GNU make支持使用`\`反斜杠符号链接下一行。
将一行分为多行的方法取决于是否在写配方中，要是在写配方中，那么分行会有所不同，之后会讨论。
若是不在写配方中，那么`make`会把反斜杠`\`和换行符都会被转换为一个单独的空格，之后所有在反斜杠和换行符周围的所有空格都会被浓缩为一个空格。

```Makefile
var := one$\
    word
```

会被转换为

```Makefile
var := one$ word
```

由于空格这个变量展开是空白，所以在变量展开后

```Makefile
var := oneword
```

### 包含别的Makefile

```Makefile
include Makefilenames
```

使用`include`指令就可以告诉`make`在读取到这一行时暂停，读取别的Makefile，读取完毕后再继续运行。`Makefilenames`可以包含`shell`脚本匹配模式。如果文件名为空，那么不会有文件会被包含，也不会报错。
比如，要是我们有三个额外的`Makefile`文件，分别是`a.mk`,`b.mk`,`c.mk`,还有一个变量`bar`展开为`bish bash`,那么

```Makefile
include *.mk $(bar)
```

扩展为

```Makefile
include a.mk b.mk c.mk bish bash
```

在运行`make`的时候加上`-I`就可以指定Makefile的搜索目录。你也可以使用`-I-`命令，取消之前指定的所有Makefile搜索目录以及默认会搜索的目录（比如/usr/local/include），但是当前运行`make`的文件目录永远都会搜索。
要是指定的Makefile并不存在于所有的搜索路径，这时Make**不会**立即报错，而是会继续输出接下来要包含的Makefile，当处理完`include`命令后，`make`会尝试重新生成过期或者是不存在的Makefile，只有当`make`没有找到规则来生成Makefile，或者是生成失败，此时才会报错。也可以指定`make`不要报未存在Makefile这个错误，使用`-include`替代`include`即可。

### 环境变量MAKEFILES

要是环境变量MAKEFILES被定义了，`make`就会认为它的值代表了一系列的`Makefile`名字，`make`就会搜索并包含这些`Makefile`，但是在没有找到时不会报错。

### make程序如何处理Makefile

默认情况下，make遇到的以一个不是`.`开头的目标就是默认目标，直接使用`make`命令就是指定的这个目标。
我们也可以通过`make target`指定一个目标作为此次运行最终要产生的目标。
通过`make target`指定目标后，make开始**读取**Makefile，读取完毕后，建立一个**依赖关系图表**，之后，先开始考虑是否需要**重建**包含的Makefile，进行处理。接着开始考虑构建指定目标`target`，从指定目标`target`的开始，循着依赖关系图表，找到图表末端的**显式规则目标**，考虑这个文件是否存在，若是存在，则向上一层，考虑以它为依赖的规则目标，看这些规则目标是否**存在**或者是比它要**旧**，考虑是否运行配方更新这些规则目标；若是不存在，则考虑是否存在什么**隐式规则**可以构建它，若有，则应用，若无，则报错。
以此类推，当这些上一层的规则目标完全被考虑之后，接着向上上层运行，直到运行到指定的`target`时，此时若是没有报错，则make构建目标**成功**，正常退出。

### make程序如何重建Makefile

有时，Makefile本身也会被重建，比如在后文提到的自动生成依赖处，修改的源文件对应的Makefile就会被重建。
在make读完所有Makefile之后，也就是下文提到的第一步完成之后，make会认为所有读取的makefile都是一个规则目标，并尝试使用显式规则或者是隐式规则重建它们。如果确实有makefile需要更新，那么make运行配方更新这个makefile。之后，make**从头开始**，从新读取所有的makefile，重复上述流程，直到**所有的**makefile都不需要更新为止，每次重启make过程，都会导致**MAKE_RESTARTS**这个变量被更新。
要是我们不想更新某个Makefile，为了防止make给makefile搜索隐式规则，我们可以**显式定义规则**，并附上空的配方。
如果将某个Makefile声明为伪目标，由于伪目标永远都不会被认为是最新的，所以make会无限地重启并一直更新，为了防止这个问题，make不会**尝试**重建声明为伪目标地makefile.

### make程序如何读取Makefile

GNU `make`程序读取Makefile是会分为明显的**两个**步骤，第一步，`make`全文读取makefiles，若是指定了`include`,还会包含别的Makefile，初始化所有的变量和它们的值，初始化隐式和显式规则，然后为所有的目标和先决条件建立一个依赖关系图表。在第二步中，`make`使用这些初始化数据决定哪个目标需要被更新，并且运行对应的配方去更新它。
理解这两个步骤有助于我们理解变量和函数是如何展开的，变量和函数的展开方式分为立即(immdediate)和延迟(deferred)。立即展开的变量是在`make`的第一步就展开了，当`make`读取到立即展开变量的这一行时，它就会直接将变量展开成对应的字符串。不是立即展开的变量就是延迟展开的变量，延迟展开的变量会直到这个变量真正被使用后（当它被一个立即展开的语法引用时或者是在`make`第二步被用到时）才会展开成特定的字符串，在之前一直保持着一个变量的状态。

#### 变量定义

以下是变量定义的展开方式,`make`会以如下的方法读取变量

```text
immediate = deferred
immediate ?= deferred
immediate := immediate
immediate ::= immediate
immediate :::= immediate-with-escape
immediate += deferred or immediate
immediate != immediate

define immediate
  deferred
endef

define immediate =
  deferred
endef

define immediate ?=
  deferred
endef

define immediate :=
  immediate
endef

define immediate ::=
  immediate
endef

define immediate :::=
  immediate-with-escape
endef

define immediate +=
  deferred or immediate
endef

define immediate !=
  immediate
endef
```

这个表格的意思是，例如当`make`读取到`$(var_imd) = $(var_def)`，它会把左侧的变量立即展开，而右侧的变量延迟展开，这样，在这一行之后修改`var_imd`不会影响这一行的`var_imd`展开的字符串值，但是在这一行后修改`var_def`就会影响这一行的`var_def`的字符串值。
对于扩展运算符`+=`,当这个变量之前是简单变量时(`:=`),那么就是立即展开的，否则就是延迟展开的。

#### 条件指令

条件指令是立即展开的，这意味着，不能在条件指令中使用自动化变量，因为自动化变量是在规则配方运行时才有值的，当然也可以在规则配方中使用条件指令的自动化变量，这是自动化变量是有值的。

#### 规则定义

```text
immediate : immediate ; deferred
        deferred
```

规则的展开方式如上，也就是目标和依赖是立即展开的，但是构建规则的配方却经常是延迟的。

### make程序如何解析Makefile

GNU make**按行**解析Makefile，这个解析的过程发生在`make`的第一步中,处理步骤如下

1. 读取整个逻辑行（指的是使用反斜杠`\`加上换行符的那些行作为一个整体的逻辑行）。
2. 移除注释
3. 要是这一行以配方前缀开始且正在处理规则，那么把这行加到目前的配方中，读取下一行。
4. 将这一行所有的立即展开的变量展开。
5. 寻找这一行是否有特殊的分隔符，比如`:`或者`=`,确定这一行是变量赋值还是规则。
6. 内化上述的结果，开始读取下一行。

### make程序二次展开

二次展开可以在规则的依赖处使用自动化变量和函数了，有时十分方便。

```Makefile
.SECONDEXPANSION:
main_OBJS := main.o try.o test.o
lib_OBJS := lib.o api.o

main lib: $$($$@_OBJS)
```

在第一步展开时，依赖变为`$($@_OBJS)`,在二次展开时，二次展开发生于第二步，所以此时自动化变量已经有值了，展开后的结果就是`$(main_OBJS)`最后展开成`main.o try.o test.o`.

也可以混合函数和变量

```Makefile
main_SRCS := main.c try.c test.c
lib_SRCS := lib.c api.c

.SECONDEXPANSION:
main lib: $$(patsubst %.c,%.o,$$($$@_SRCS))
```

## 规则写法

规则指导make程序什么时候以及怎么样重建规则目标，声明规则目标依赖，声明构建规则目标的配方。

参考文档

* GNU make官方文档[GNU make Writing Rules](https://www.gnu.org/software/make/manual/make.html#Rules)

除了默认规则，其余规则书写的顺序不是特别重要。默认规则就是`make`读取到的第一个规则，它会在我们没有指定`make`的目标时自动认为指定的是默认的规则目标。

### 规则语法

规则的语法如下

```text
targets : prerequisites
  recipe
  …

targets : prerequisites ; recipe
  recipe
  …
```

`targets`是规则目标，通常是文件名，使用空格分隔，可以使用通配符(`%`)。通常情况下每个规则只有一个规则目标，但是有时也可能会知道多个目标，指定多个目标的语法不是上述所示的语法，如果使用上述的语法，那么`make`还是认为每个规则只有**一个**规则目标，从而会自动生成**多个**规则，每个规则各自有一个不同的`target`.
为同一个规则目标设置多个规则是可以的，但是只能有**一个**规则指定配方，`make`会把这些多个规则集合成一个规则，每个规则的依赖都是最终集合成的规则的依赖。
`recipe`行必须以一个`tab`键开始（或者`.RECIPEPREFIX`变量开始）。第一个配方可以正好在依赖后面，使用分号`;`分隔。配方会被传递到`shell`，所以配方实际上是`shell`脚本指令。
判断规则目标是否存在是看它是否存在于对应的文件目录中，也就是说`make`认为每个规则目标都是**文件**.
判断规则目标是否过时就是看规则目标的文件的时间戳是否晚于任何一个依赖的时间戳，若是，则规则目标过时，需要重新构建。

### 依赖类型

有两种不同的依赖类型，一种是**普通依赖**(normal prerequisties)，一种是仅**顺序依赖**(order-only prerequistites).
普通依赖就是前文一直提过的依赖类型。首先，普通依赖影响配方运行的顺序，也就是说，只有当所有依赖的配方以及运行完毕后，才会轮到当前规则目标的配方，所有的依赖也有可能是别的规则的规则目标，这样递归地运行。其次，普通依赖影响了依赖关系，也就是说，要是任何依赖比规则目标新，那么规则目标就会被认为是过时的，从而被重建。
仅顺序依赖则不同，仅顺序依赖只保证所有的仅顺序依赖都会在规则目标前被构建，但是当仅顺序依赖比规则目标新时，它不会强制要求规则目标重新构建。

```test
targets : normal-prerequisites | order-only-prerequisites
```

仅顺序依赖可以用在这种情况，规则目标放在不同的目录，目录可能在make运行前不存在。在这个情况下，我们想要这个目录在任何规则目标被创建前被创建，由于文件目录的时间戳会随着它里面的文件添加，移动或者重命名而改变，但我们不希望当目录的时间戳改变时就重建所有的规则目标。那么我们就可以使用仅顺序依赖。

```Makefile
OBJDIR := objdir
OBJS := $(addprefix $(OBJDIR)/,foo.o bar.o baz.o)

$(OBJDIR)/%.o : %.c
  $(COMPILE.c) $(OUTPUT_OPTION) $<

all: $(OBJS)

$(OBJS): | $(OBJDIR)

$(OBJDIR):
  mkdir $(OBJDIR)
```

### 在文件名中使用通配符

使用通配符可以方便地指定一类文件，通配符包含`*`,`?`,`[...]`。当通配符无法匹配任何一个文件时，`make`假定通配符被**取消**从而直接寻找，比如`*.o`没有匹配任何`.o`文件，那么`make`会直接匹配`*.o`这个有星号的文件。
通配符匹配的多个文件会被排序，但是多个通配符匹配的文件不会整体地排序。比如`*.c *.h`,`make`会首先会匹配工作目录里所有的以`.c`结尾的文件，之后对这些文件排序，然后匹配工作目录中所有以`.h`结尾的文件，对这些文件排序，但**不会**整体地排序。
符号`~`出现在文件名前有着特殊的意义，如果是`~/filename`，那么这个代表着linux里home目录的当前用户，比如`/home/hitman/filename`。如果是`~word/filename`，这个表示的是linux里的home目录的`word`用户，比如`/home/word/filename`.
通配符在规则目标和依赖里是由`make`程序负责自动展开，在配方里的通配符则是由`shell`终端负责展开的。而在规则之外，**除非**显式地使用`wildcard`函数，否则通配符**不会**展开（比如变量定义里使用通配符）。
可以用`\`取消通配符，比如`foo\*bar`只会匹配`foo*bar`.

* `*`表示匹配所有字符串，除了`\`符号，所以`*.c`就会匹配当前目录下的所有以`.c`结尾的文件，但不会匹配子目录下的`.c`文件。
* `%`表示匹配所有的字符串并提取出来，之后可以把提取出来的字符串用于依赖中。这样可以方便地指定多个相同模式的规则。`%.o : %.c`就会生成所有`.o`结尾文件的规则，同时这个规则的依赖`.o`文件的同名`.c`文件。
* `?`表示匹配一个字符，除了`\`符号。
* `[...]`表示匹配一系列在`[]`内的一个字符。

#### wildcard函数

通配符只会在规则里面自动展开，但是不会在别的地方自动展开，所以需要展开时需要使用`wildcard`函数

```Makefile
$(wildcard pattern...)
```

这个函数会把展开`pattern`,把它用具体的文件名替换，每个文件名之间采用空格分隔开。要是没有匹配的文件，那么`pattern`不会输出,也就是输出空字符串，这个于wildcard在规则的的行为**不同**。

当然，也可以把`wildcard`函数和别的函数结合起来，比如下边的这个函数就是匹配所有`.c`文件并将它们替换为对应的`.o`文件。

```Makefile
$(patsubst %.c,%.o,$(wildcard *.c))
```

我们可以用这个函数简单地连接所有编译出的`.o`文件

```Makefile
objects := $(patsubst %.c,%.o,$(wildcard *.c))

foo : $(objects)
  cc -o foo $(objects)
```

### 搜索依赖的文件目录

对于大型项目，通常把源文件放在不同的文件夹中，把二进制文件单独存放，`make`可以指定多个依赖的搜索路径。当修改了文件的位置时，不必更改规则，直接修改搜索路径即可。

#### 使用VPATH环境变量指定搜索依赖和规则目标的目录

特殊的变量`VAPTH`指定了`make`搜索**依赖**和**规则目标**路径。所以，要是指定的文件名没有在当前的文件目录中，`make`就会搜索`VPATH`指定的文件目录，就好像这些文件存储在当前目录中一样。
使用冒号`:`或者是空格分隔`VPATH`指定的搜索目录，目录出现在`VPATH`的顺序就是`make`搜索文件的顺序。

```Makefile
VPATH := src ../headers
```

上述Makefile指定了两个额外搜索路径，一个是`src`,另一个是`../headers`.

#### 使用vpath指令指定搜索的依赖目录

使用vpath指令可以指定一类文件的搜索目录而不影响另一类文件。

* `vpath pattern directories`为满足`pattern`文件加上额外的搜索路径`directories`。
* `vpath pattern`清除符合模式`pattern`文件之前使用`vpath`指令设置的额外搜索路径。
* `vpath`清理所有文件之前使用`vpath`指令设置的额外搜索路径。

```Makefile
vpath %.h ../headers
```

#### 目录搜索是如何进行的

但找到了依赖后，文件名就不是我们在Makefile指定的了（还要加上路径），但有时候，搜索到的路径会被`make`丢弃、

`make`使用如下的方法进行目录搜索，同时判断是否需要保持或者是抛弃文件路径。

1. 要是目标文件不存在与当前目录，就会开始目录搜索，（使用VPATH变量指定的位置）。
2. 要是目录搜索成功，则保留该路径，并将这个带有路径的文件作为目标替换原来没有路径的文件。
3. 要是依赖不存在与当前目录，就会开始目录搜索，（使用VPATH变量指定的位置）。
4. 要是目录搜索成功，则保留该路径，并将这个带有路径的文件作为依赖替换原来没有路径的文件。
5. 在处理完毕依赖后，决定是不是要重建目标，1)要是目标**不需要**被重建，那么不会丢弃这些路径。2）要是目标**需要**被重建，那么路径就会被**丢弃**，目标就会使用在Makefile里指定的文件名重建。但是依赖的路径**不会**被丢弃，但是要指定带有文件名的依赖，需要使用自动化变量。

当然，你可以使用`GPATH`指定不会被丢弃的路径。

#### 如何在目录搜索的前提下写配方

由于前文所说的，当规则目标过期之后，需要运行配方更新规则目标时，搜索到的路径被抛弃了，还是使用依赖在Makefile里写的文件名，所以配方写成可以查找对应的文件目录的格式。
使用自动化变量就可以轻松地实现我们的要求。

```Makefile
foo.o : foo.c
  cc -c $(CFLAGS) $^ -o $@
```

上文使用自动化变量，`$^`表示带有路径的依赖文件名，`$@`则是目标名，但是**没有**路径.

### 伪目标

伪目标指的是那些不是真正的文件的目标。它可能只是一个名字，用来指示`make`处理特定的命令。有两种原因使用伪目标，1）避免与一个同名的文件产生冲突。2）提高性能
要是我们写一个不创造规则目标文件的规则，那么这个规则目标就总是会被重建。

```Makefile
clean:
  rm *.o temp
```

因为`rm`命令没有创建一个名为`clean`的文件，那么当我们键入`make clean`时，`rm`命令总是会被运行。
假如文件目录中存在一个名为`clean`的文件，因为它没有依赖，所以`clean`会被认为没有过时，从而它的配方不会运行。为了避免上述问题的产生，我们可以通过将规则目标作为一个特殊的目标`.PHONY`的**依赖**从而显式地将规则目标定义为**伪目标**。

```Makefile
.PHONY: clean
clean:
  rm *.o temp
```

`.PHONY`的依赖永远都会被翻译为字面值，也就是说不能使用通配符进行模式匹配。为了匹配一个模式，使用`force`目标。

伪目标在递归地调用`make`时很有用，它可以更好地报告错误以及达到更优越的并行性能。

```Makefile
SUBDIRS = foo bar baz

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
  $(MAKE) -C $@

foo: baz
```

我们使用`foo: baz`告诉`make`不要重建`foo`直到`baz`重建完成。我们就可以使用make的并行性能加快运行速度。
伪目标**不应该**成为一个真正目标文件的依赖，否则，每当`make`考虑到了这个文件，这个规则目标的配方就总是会被运行。
伪目标本身也可以有依赖，当我们想要一个Makefile生成多个程序时，我们通常声明`all`作为伪目标。

```Makefile
all : prog1 prog2 prog3
.PHONY : all

prog1 : prog1.o utils.o
  cc -o prog1 prog1.o utils.o

prog2 : prog2.o
  cc -o prog2 prog2.o

prog3 : prog3.o sort.o utils.o
  cc -o prog3 prog3.o sort.o utils.o
```

这样，我们使用`make`命令就可以一口气生成三个程序，当然，也可以使用`make prog1`只生成一个程序。伪目标不会遗传，伪目标的依赖不会自动成为伪目标。
当一个伪目标是另一个伪目标的依赖时，这个伪目标就相当于另一个伪目标的子程序。

```Makefile
.PHONY: cleanall cleanobj cleandiff

cleanall : cleanobj cleandiff
  rm program

cleanobj :
  rm *.o

cleandiff :
  rm *.diff
```

### 没有配方或者是依赖的规则

对于一个没有配方或者是依赖的规则，而且它的规则目标的文件不存在，那么`make`在其规则运行时就会假定该目标已经更新了。这个表示所有依赖于它的目标**都会**运行配方。

```Makefile
clean: FORCE
  rm $(objects)
FORCE:
```

### 特殊用途的目标名

有一些make用于特殊用途的目标名，将目标声明为这些的依赖就意味着这些目标具有某种属性。

* `.PHONY`
  `.PHONY`的依赖会被认为是**伪目标**。make会认为这些伪目标永远都是**过时**(out-of-date)的，也就是说，当需要考虑伪目标时，make总是会运行这些规则目标的配方，不管这个目标文件是否存在或者它上次修改的时间。

### 静态模式规则

静态模式规则(static pattern rules)可以方便地只用一个规则就可以声明一类的规则的方法。

#### 静态模式规则语法

```text
targets …: target-pattern: prereq-patterns …
  recipe
  …
```

`targets`就是规则目标，可以使用**通配符**。
`targets-pattern`和`prereq-patterns`告诉了make程序如何依照规则目标自动地生成对应的依赖。每个规则规则目标会与`target-patterns`匹配的模式，提取一部分的目标名称出来，叫做词干(stem),词干之后就会替换对应的`prereq-patterns`里的模式，从而生成依赖。
使用`%`就可以定义模式，匹配任意长度的字符串。

```Makefile
objects = foo.o bar.o

all: $(objects)

$(objects): %.o: %.c
  $(CC) -c $(CFLAGS) $< -o $@
```

上述就是静态规则模式的一个例子，对于规则目标`foo.o`，make将匹配的词干`foo`提出来，把它和`prereq-patterns`里的模式`%`替换，生成对应的依赖`foo.c`.自动化变量`$<`就是依赖的名字，`$@`就是目标的名字。
每个目标都要可以匹配规则模式，若是有一个规则目标不匹配，就会产生错误信息。如果有一系列的文件，只有一些可以匹配规则模式，那么可以先使用`filter`函数滤除不匹配的文件名。

```Makefile
files = foo.elc bar.o lose.o

$(filter %.o,$(files)): %.o: %.c
  $(CC) -c $(CFLAGS) $< -o $@
$(filter %.elc,$(files)): %.elc: %.el
  emacs -f batch-byte-compile $<
```

### 自动产生依赖

在实际写Makefile中，C/C++的源文件还会依赖许多.h头文件。但是，对于大量的C源文件，每个都手动写头文件依赖十分麻烦而且不太现实，因为每次修改了头文件就要重新编写Makefile。
为了避免这种情况，大部分的现代C/C++编译器都提供了一个自动生成包含头文件的一个文件，通过`cc -M main.c`命令。
在GNU make中，我们通常会为每个源文件都创建一个对应的Makefile文件，比如对于源文件`name.c`会创建一个`name.d`文件，这个文件列出了`name.o`依赖的文件，这样，只有修改了的源文件才会被搜索从而产生依赖。

```Makefile
%.d: %.c
  @set -e; rm -f $@; \
  $(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
  rm -f $@.$$$$
```

以源文件`name.c`为例。首先，使用`set -e`命令告诉shell要是`$(CC)`遇到错误后立即退出。之后删除了`name.d`。之后使用命令生成头文件依赖，并将输出暂时存入`name.d.$$`。之后使用`sed`命令替换字符串，将`name.o:`替换为`name.o name.d :`将结果存入`name.d`中，最后删除`name.d.$$`.
`sed`命令的目的是将

```Makefile
main.o : main.c defs.h
```

翻译为

```Makefile
main.o main.d : main.c defs.h
```

它让每个`.d`都依赖于源文件和头文件，由于头文件内也可能包含头文件，防止了包含的头文件被修改但是没有重新生成`.d`文件的问题。
当已经定义了一个规则去重新生成`.d`文件，可以只用命令`include`将这些`.d`文件包含进入Makefile。

```Makefile
source = foo.c bar.c

include $(sources:.c=.d)
```

这个例子使用了一个替换变量的方法，把所有`.c`文件替换为`.d`文件。由于`.d`文件也是Makefile，make程序会在必要的时候重新开始make一遍。

## 规则配方写法

规则配方实际上就是一个或者多个shell命令，make不会尝试去理解规则配方的写法，它只会做一些简单地翻译工作，比如自动化变量，变量展开等，将开头的`tab`键删去（只删去一个），之后就会把它传递到shell中。

* GNU 官方文档[GNU make](https://www.gnu.org/software/make/manual/make.html#Recipes)

### 规则配方语法

除了第一个规则配方可以用分号分隔开来，直接跟在依赖后面。其它配方必须以一个`tab`键开始，并且出现在规则语义上下文中。

### 将一行配方分为多行

将配方一行分为多行时的方法和在Makefile别的地方不一样。在规则配方中，反斜杠加上换行符**不会**被make移去，而是直接将其原封不动地传输给shell，注意下一行的`tab`键直接被删去了，**不会**加上空格。

```Makefile
all :
  @echo no\
  space
  @echo no\
  space
  @echo one \
  space
  @echo one\
  space
```

会输出

```text
nospace
nospace
one space
one space
```

### 在规则配方中使用变量

可以在规则配方中使用变量，make会展开变量，但是展开变量的事情发生在make读取了所有的Makefile并且决定重建规则目标之后才会展开这个变量。
如果想在shell中使用真正的`$`，那么必须使用`$$`,这样，make展开变量`$$`为`$`。

```Makefile
LIST = one two three
all:
  for i in $(LIST); do \
    echo $$i; \
  done
```

经过make的翻译后，实际传输到make的内容如下

```shell
for i in one two three; do \
    echo $i; \
done
```

### 配方回声

通常情况在运行每一行的配方前，make会打印出来，这个就叫做回声`echoing`。
我们也可以使用`@`不产生配方回声，`@`会在这一行被传输到shell前被删除。

```Makefile
@echo About to make distribution files
```

使用命令行参数`-n`就可以只**回声**配方（包括已经使用`@`禁用回声的哪一行）而不是实际运行它。
使用命令行参数`-s`就可以不产生所有的回声。

### 配方运行

当make决定该运行配方更新目标时，它就会给每一行配方**分别**创建一个子shell，这意味着，假如一行使用`cd`,那么另一行**不会**受到影响。

```Makefile
foo : bar/lose
  cd $(<D) && gobble $(<F) > ../$@
```

上述配方中使用了shell的连接符`&&`，在一行中运行了几个shell命令。

### 配方中的错误信息

在每个shell结束运行后，make都会观察它的返回状态，要是shell成功执行(返回值为0),那么下一行配方就会在新的一个shell中执行，直到所有的配方完成。但是要是出现了**错误**(返回值非零)，那么make会放弃目前的规则，也很有可能放弃整个make的运行。
<<<<<<< HEAD
有时，我们想要某一行配方发生了错误也继续运行，我们可以在想要忽略的配方前加上`-`.

```Makefile
clean:
  -rm -f *.o
```

### 递归使用make

递归使用make意味着把make作为配方中出现的命令使用。这个方法主要用于维护不同的子系统的Makefile的。比如，假定一个子文件夹`subdir`有它自己的Makefile，我们需要在这个子文件夹里也运行make。

```Makefile
subsystem:
  cd subdir && $(MAKE)
```

或者是

```Makefile
subsystem:
  $(MAKE) -C subdir
```

当make运行时，当处理到`-c`命令时，make就会设置变量`CURDIR`

#### 传递给子make变量

可以显式地要求顶端make的变量可以被传输到子make。这些变量会默认也在子make中定义，但是不会覆写子make的同名变量（可以使用`-e`覆写）。
make会把所有传递下去的变量和它的值加到每一行运行配方时的环境中，这样，子make就会将这些变量作为环境变量，不仅如此子make的子make也会
除了显式传递变量，make还会传递起初的环境变量，或者是在命令行设置的变量。事实上，make会把所有在命令行设置的变量都加入到`MAKEFLAGS`变量里，并总是传递`MAKEFLAGS`这个变量。
我们可以使用`export`命令显式地传递某个变量。

```Makefile
export variable …
```

我们也可以使用`unexport`命令停止传递某个变量。

```Makefile
unexport variable …
```

由于`unexport`是默认的行为，我们只有在取消之前用过的`export`才会使用.
每次递归地调用make变量`MAKELEAVEL`的值都会自增一。
`export`会展开变量名和函数，也就是说`export`右端的是**立即展开**的，这**不意味着**`export`在使用这里立即传递变量，而是等到本Makefile被解析完成，才实际开始传递变量。

```Makefile
export_val1 = first_export_val
export export_val1
export_val1 = first_export_val_changed

export_val2 := second_export_val
export export_val2
export_val2 := second_export_val_changed

export_val3 = third_export_val
export export_val3
export_val3 += third_export_val_add

export_val4 = forth_export_val
export_val5 = fifth_export_val

name_of_export_val = export_val4
export $(name_of_export_val)
name_of_export_val = export_val5

submake: force
  cd sub_make_dir && $(MAKE)

.PHONY: force
force:

```

在子Makefile内容如下

```Makefile
submake: force
  echo $(export_val1)
  echo $(export_val2)
  echo $(export_val3)
  echo $(export_val5)
.PHONY: force
force:
```

输出为

```text
cd sub_make_dir && make
make[1]: Entering directory '/home/hitman/MakefileTest/sub_make_dir'
echo first_export_val_changed
first_export_val_changed
echo second_export_val_changed
second_export_val_changed
echo third_export_val third_export_val_add
third_export_val third_export_val_add
echo 

make[1]: Leaving directory '/home/hitman/MakefileTest/sub_make_dir'
```

### 定义一个配方集合

我们可以定义一个配方集合用于多个地方，方法和使用变量差不多。

```Makefile
define run-yacc =
yacc $(firstword $^)
mv y.tab.c $@
endef
```

在这里，`run-yacc`就是我们定义的配方集合，方法和定义多行变量一致。从`define`命令开始，到`endef`结束，在`define`中，make不会展开任何的`$`，也就是说，在这里面的所有符号全部都是字面意义。

```Makefile
foo.c : foo.y
  $(run-yacc)
```

我们可以如上所示地使用配方集合，此时所有在`define`里使用的变量才会开始展开。
在配方运行时，配方集合的每一行都相当于前面加上`tab`键，直接出现在了规则配方之内。

### 使用空的配方

有时使用空配方是有用的，空配方第一个用处是用于避免make的隐式规则，空配方第二个用处是可以用来避免规则目标是别的目标生成时的副产物的错误。

```Makefile
target: ;
```

## 如何使用变量

变量是一个名字，代表着特定的字符串，会在特定时候被替换为对应的字符串。

参考资料

* [GNU make](https://www.gnu.org/software/make/manual/make.html#Using-Variables)

```Makefile
objects = program.o foo.o utils.o
program : $(objects)
        cc -o program $(objects)

$(objects) : defs.h
```

上文便是使用了变量进行替换objects。

### 变量的两种风格

GNU make使用不同的方式（风格）去得到变量的值。

#### 递归展开的变量

```Makefile
foo = $(bar)
bar = $(ugh)
ugh = Huh?

all:;echo $(foo)
```

由于make处理流程，当运行`all`规则时，变量`foo`先展开成`$(bar)`再展开成`$(ugh)`最后展开成`Huh?`，这就是递归地展开了变量。

#### 简单展开的变量

```Makefile
x := foo
y := $(x) bar
x := later
```

等价于

```Makefile
y := foo bar
x := later
```

#### 条件定义变量

```Makefile
FOO ?= bar
```

等价于

```Makefile
ifeq ($(origin FOO), undefined)
  FOO = bar
endif
```

只有在`FOO`未定义的情况下，才会定义FOO。

### 变量的进阶特性

可以使用变量的进阶特性灵活地定义变量。

#### 先代换后赋值

```Makefile
foo := a.o b.o l.a c.o
bar := $(foo:.o=.c)
```

上面的Makefile设置`bar`为`a.c b.c l.a c.c`使用`$(var:a=b)`这个格式，他会替换每个单词后面的`a`换成`b`.
这个替换是函数`patsubst`的简写形式，`$(var:a=b)`就是`$(patsubst %a,%b,var)`.

```Makefile
foo := a.o b.o l.a c.o
bar := $(foo:%.o=%.c)
```

这个是完整的形式。

#### 计算变量名字

```Makefile
x = y
y = z
a := $($(x))
```

上述最终定义`a`为`z`,把一个变量的值有当做是另一个变量的名字使用。

```Makefile
x = $(y)
y = z
z = Hello
a := $($(x))
```

最终定义`a`为`Hello`，首先`$(x)`展开为`$(y)`,展开为`$(z)`,最后是`Hello`.

```Makefile
a_objects := a.o b.o c.o
1_objects := 1.o 2.o 3.o

sources := $($(a1)_objects:.o=.c)
```

使用变量决定变量名字的一部分也十分常用。

### 给一个变量加上新的文本

```Makefile
objects = main.o foo.o bar.o utils.o
objects += another.o
```

使用`+=`给变量加上新的文本，注意前面的空格，没有空格的话就是直接加在上一个单词的后面。

### 定义多行变量

使用`define`命令就可以定义多行的变量。make会删除最后一个换行符，并将在`define`和`endef`里的内容作为变量的值。

```Makefile
define newline


endef
```

由于make会删除最后一个换行符，所以为了定义一个值为换行符的变量，需要两行。

```Makefile
define variable =
variable_string
endef
```

### 未定义的变量

如果想要清空变量，可以简单地把变量的值设置为空字符串。但是清空变量与未定义变量是不同的，这一点体现在函数`origin`的结果上。
如果想要停止定义变量，使用`undefine`命令就可以了。

```Makefile
foo := foo
bar = bar

undefine foo
undefine bar

$(info $(origin foo))
$(info $(flavor bar))
```

会输出`undefined`结果。

### 来自环境的变量

变量在make启动时就存在的变量，make会把它们存入同名同值的变量中。也就是说，除了使用特定指令，在Makefile里是无法修改环境变量的，只会修改与它同名同值的本地变量。

### 只用于特定规则的变量

```Makefile
target : variable-assignment
```

可以在规则定义处定义变量，这样这个变量就会覆盖原全局变量或者是产生一个新的局部变量。

```Makefile
prog : CFLAGS = -g
prog : prog.o foo.o bar.o
```

上面的Makefile就在`prog`规则中定义了一个覆盖了全局变量`CFLAG`的同名局部变量，在`prog`的配方中就可以使用了。

## Makefile条件指令

条件指令可以使得Make根据情况自动选择Makefile的内容，加大了灵活性。

```Makefile
libs_for_gcc = -lgnu
normal_libs =

foo: $(objects)
ifeq ($(CC),gcc)
  $(CC) -o foo $(objects) $(libs_for_gcc)
else
  $(CC) -o foo $(objects) $(normal_libs)
endif
```

条件指令`ifeq`,`else`,`endif`实现了条件控制。

### 条件指令语法

完整的条件指令语法如下

```Makefile
conditional-directive-one
text-if-one-is-true
else conditional-directive-two
text-if-two-is-true
else
text-if-one-and-two-are-false
endif
```

```Makefile
ifeq (arg1, arg2)
ifneq (arg1, arg2)
ifdef variable-name
ifndef variable-name
```

在条件指令这一行，额外的空格会被忽略，但是`tab`是不允许出现的。
make处理条件指令是立即的，也就是说读到这一行条件指令之后，make立刻展开变量，进行条件比较，为真，则读取为真时的条件内容，反之亦然。

## 函数

函数在Makefile中十分有用，可以使用函数进行文本处理或者是运行特定的操作。
make定义了一系列函数，足够大部分的使用场景，如果还是有特定需求，那么可以定义自己的函数。

### 函数调用语法

```Makefile
$(function arguments)
```

函数调用语法和使用变量十分相像。
`function`是函数名字，可以是一系列make预先定义的函数名字，也可以使用`call`来调用自身的函数。
`arguments`是函数参数，它们与函数名字之间使用一个或者多个空格或者是`tab`来分隔。参数之间使用逗号`,`来分隔。这些空格和逗号不是参数值的一部分，只是起到了分隔的作用。

### 用于字符串替换的函数

#### $(subst from,to,text)

从`text`中将所有`from`替换为`to`.

```Makefile
$(subst ee,EE,feet on the street)
```

结果就是`fEEt on the strEEt`.

#### $(patsubst pattern,replacement,text)

寻找以空格区分开来的单词并进行模式替换，从`text`中替换符合模式`pattern`为`replacement`.模式匹配包含`%`，匹配并提取任何长度的字符串（不包括空格），可以被用于`replacement`中。
可以使用`\%`取消这个字符模式匹配,不匹配的单词**原样**输出。
进行匹配后，在单词之间的多个空格会被替换为**单个空格**，并且**去除**所有的前导和尾随空格。

```Makefile
$(patsubst %.c,%.o,x.c.c bar.c bar)
```

会输出字符串`x.c.o bar.o bar`.

#### $(strip string)

从字符串中去除前导和尾随的空格，将内部的多个空格替换为单个空格。

```Makefile
$(strip a b c )
```

会输出字符串`a b c`.

通常这个函数在使用条件判断时很有用，当一个只有空白的字符的变量认为是空变量，这样使用`strip`就很实用。

#### $(findstring find,in)

寻找`find`，若是找到就返回若是没找到就输出空字符串。

```Makefile
$(findstring a,a b c)
$(findstring a,b c)
```

分别输出`a`和空字符串''.

#### $(filter pattern…,text)

返回所有以空格分隔的`text`的匹配`pattern`的单词，并保持原顺序。可以指定多个`pattern`相互间用空格隔开。

```Makefile
sources := foo.c bar.c baz.s ugh.h
foo: $(sources)
  cc $(filter %.c %.s,$(sources)) -o foo
```

输出`foo.c bar.c baz.s`作为cc的编译源文件。

#### $(filter-out pattern…,text)

和上面的函数相反，返回所有**没有**匹配的单词。

#### $(sort list)

对`list`以字典序排序，去除重复的单词，同时去除多个空格。

#### $(word n,text)

返回`text`的第n个单词，若是没有则返回空字符串。

#### $(wordlist s,e,text)

返回第s个和第e个单词之间的单词（包含其本身）。

```Makefile
$(wordlist 2, 3, foo bar baz)
```

返回`bar baz`.

#### $(words text)

返回有多少个单词。

```Makefile
$(word $(words text),text).
```

这个就会返回`text`最后一个单词。

#### $(firstword names…)

返回第一个单词。

```Makefile
$(firstword foo bar)
```

返回`foo`

#### $(lastword names…)

返回最后一个单词。

#### 函数实例

下面有个实例，使用的就是函数`patsubst`。假定Makefile使用`VPATH`变量去指定make搜索依赖的的目录。这个例子展示了如何告诉C编译器去搜索头文件。

如果VPATH是以分号`;`分割的，首先应该把分号替换为空格。

```Makefile
$(subst :, ,$(VPATH))
```

之后，为了适应C编译器的要求，每个文件目录前面都应该加上`-I`。

```Makefile
override CFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))
```

### 用于取得文件名的函数

#### $(dir names…)

提取出`name`的文件目录部分,多个文件的话，输出也是多个，各自代表对应的文件目录部分。文件目录部分就是从开始到最后一个斜杠`/`,若是没有斜杠,那么就是`./`.

```Makefile
$(dir src/foo.c hacks)
```

输出`src/ ./`.

#### $(notdir names…)

与上面的函数相反，提取出`name`的文件名部分。

```Makefile
$(notdir src/foo.c hacks)
```

输出`foo.c hacks`.

#### $(suffix names…)

提取出文件的后缀名，也就是最后一个`.`及其之后的内容，要是文件没有后缀名，那么返回空字符串。
多个文件名也会输出多个，相互间用空格隔开。

```Makefile
$(suffix src/foo.c src-1.0/bar.c hacks)
```

会输出`.c .c`.

#### $(basename names…)

遇上面函数相反，提取出除了后缀名的文件名。

```Makefile
$(basename src/foo.c src-1.0/bar hacks)
```

会输出`src/foo src-1.0/bar hacks`.

#### $(addsuffix suffix,names…)

给文件加上后缀名。

```Makefile
$(addsuffix .c,foo bar)
```

输出`foo.c bar.c`

#### $(addprefix prefix,names…)

给文件加上前缀。

```Makefile
$(addprefix src/,foo bar)
```

输出`src/foo src/bar`

#### $(join list1,list2)

将`list1`和`list2`的单词分别连接起来，`list1`的第一个单词和`list2`的第一个单词连接，`list1`第二个单词和`list2`第二个单词连接以此类推，要是一个输入比另一个输入单词数要多，那么原封不动复制剩余的在输出字符串里。

```Makefile
$(join a b,.c .o)
```

输出`a.c b.o`

#### $(wildcard pattern)

进行模式匹配并展开，展开成以空格分隔的所有匹配模式的文件名。

#### $(realpath names…)

返回文件的真实路径名称，也就是绝对路径并且不包括符号链接。

#### $(abspath names…)

返回文件的绝对路径，但是不会求解符号链接。

### 用于条件控制的函数

#### `$(if condition,then-part[,else-part])`

`condition`参数，首先**去除**所有的前导和尾随空格，然后展开，若是它展开为**非空字符串**，那么make就认为`true`,反之则为`false`.
要是条件为`true`那么第二个参数`then-part`会展开，并作为`if`函数的返回值。
反之。要是条件为`false`那么第三个参数`else-part`会展开，并作为`if`函数的返回值，这个参数是可选的，要是不指定这个参数的话，函数返回空字符串。

#### `$(or condition1[,condition2[,condition3…]])`

`or`函数提供了一个**短路**的或运算，每个参数按顺序展开，要是一个参数展开为**非空字符串**，那么这个函数终止，返回值就是这个非空字符串。若是所有参数都是空字符串，那么返回值也是空字符串。

#### `$(and condition1[,condition2[,condition3…]])`

`and`函数提供了一个**短路**的与运算，每个参数按顺序展开，如果有一个参数展开为空字符串，那么这个函数终止，返回值为空字符串。反之，若是所有参数都不是空字符串，那么返回值就是**最后**一个参数的展开值。

#### `$(intcmp lhs,rhs[,lt-part[,eq-part[,gt-part]]])`

`intcmp`函数提供了整数的比较功能。
`lhs`与`rhs`会被展开然后认为是以10为基数的整数，剩下的参数的展开由`lhs`和`rhs`比较后的结果。
要是没有指定别的参数，那么若是两个数字不相等，函数返回空字符串，否则返回它们的数值。
要是至少指定了`lt-part`参数，若是`lhs<rhs`，那么函数返回`lt-part`展开值；若是`lhs=rhs`函数返回`eq-part`的展开值；若是`lhs>phs`函数返回`gt-part`的展开值。
如果不指定`gt-part`,那么它默认为`eq-part`.`eq-part`也不指定，那么`eq-part`默认为空字符串。

```Makefile
$(intcmp 9,7,hello)
$(intcmp 9,7,hello,world,)
$(intcmp 9,7,hello,world)
```

前两个函数返回空字符串，第三个函数返回`world`.

### foreach函数

`foreach`函数会使得一段Makefile重复执行，每次重复都会改变一些输入的字符串。

```Makefile
$(foreach var,list,text)
```

开始的`var`和`list`会首先展开，此时`text`还没有展开。之后每个在展开后的`list`里的单词会赋给`var`这个临时变量。之后展开`text`,由于`text`通常包含变量`var`，所以每次展开都不相同。
函数返回值就是每次展开后的`text`的字符串，以空格分隔。

```Makefile
dirs := a b c d
files := $(foreach dir,$(dirs),$(wildcard $(dir)/*))
```

首先展开`var`和`list`，`bar`不必展开，`$(dirs)`展开为`a b c d`.之后现将`a`赋值给`dir`,展开`$(wildcard $(dir)/*)`,变为为`$(wildcard a/*)`,最后展开成为所有a目录下的文件了。第二次，将`b`赋值给`dir`重复流程，直到所有的`list`都被遍历过了。
结果等价于

```Makefile
files := $(wildcard a/* b/* c/* d/*)
```

当`foreach`运行结束后，`var`临时变量就不复存在了，结果就是`var`变量不会影响其它的Makefile。

### call函数

`call`函数是一个独特的函数，它可以用来调用一个用户定义的函数。

```Makefile
$(call variable,param,param,…)
```

当make扩展`call`函数时，它会把分别把`param`赋值给`variable`里的临时变量`$(1)`,`$(2)`以此类推。而变量`0`就是`variable`,参数数量的多少没有要求，但是使用`call`但没有参数是没有意义的。
赋值了临时变量之后，`variable`变量就会展开.
注意，`variable`是变量的**名称**，不是变量解引用`$(var)`。
要是`variable`是一个内联函数，这个韩式总是会执行。
make会**首先**扩展`param`之后再将它们扩展后的值赋给临时变量，所以有些内联函数可能不会如我们所想的运行。

```Makefile
reverse = $(2) $(1)

foo = $(call reverse,a,b)
```

`foo`就会包含`b a`.

```Makefile
pathsearch = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(PATH)))))

LS := $(call pathsearch,ls)
```

LS包含`/bin/ls`.

`call`函数还可以递归地调用，每次递归地调用都会重设临时变量的值并屏蔽上一层调用的临时变量。

注意，当给`call`的参数加上空格时要小心，因为参数会**原封不动**的赋值给临时变量。

### value函数

`value`函数可以返回一个变量不被展开时的值，注意这个函数**不会**取消变量的展开，他只是忠实地输出当make运行到这一行时，变量的值。

```Makefile
$(value variable)
```

```Makefile
FOO = $PATH

all:
        @echo $(FOO)
        @echo $(value FOO)
```

第一行输出`ATH`因为`$P`展开为空字符串，第二行输出`$PATH`.

### eval函数

`eval`函数是十分特别的，它展开参数后，make会直接读取参数，就像它们直接写在Makefile行中一样，此外，`eval`的返回值通常是空字符串，这样它可以用在Makefile的大部分地方。

```Makefile
PROGRAMS    = server client

server_OBJS = server.o server_priv.o server_access.o
server_LIBS = priv protocol

client_OBJS = client.o client_api.o client_mem.o
client_LIBS = protocol

# Everything after this is generic

.PHONY: all
all: $(PROGRAMS)

define PROGRAM_template =
 $(1): $$($(1)_OBJS) $$($(1)_LIBS:%=-l%)
 ALL_OBJS   += $$($(1)_OBJS)
endef

$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))

$(PROGRAMS):
        $(LINK.o) $^ $(LDLIBS) -o $@

clean:
        rm -f $(ALL_OBJS) $(PROGRAMS)
```

首先`foreach`函数先运行，把`prog`赋值`server`.之后`eval`展开参数，`call`函数运行后，等价于
`$(eval  server: $(server_OBJS) $(server_LIBS:%=-l%) ALL_OBJS   += $(server_OBJS))`.之后`eval`展开变量，变为`server: server.o server_priv.o server_access.o -lpriv -lprotocol ALL_OBJS   +=  server.o server_priv.o server_access.o`传递给make读取。等价于定义了如下Makefile。

```Makefile
server: server.o server_priv.o server_access.o -lpriv -lprotocol
ALL_OBJS   +=  server.o server_priv.o server_access.o
```

之后`eval`返回空字符串，不影响其余函数的处理。

```Makefile
A = aaa
B = bbb
$(eval A += $B)
```

由于`A`和`B`都是延迟展开的，为了让`B`变成立即展开，防止`A+=$B`B是延迟展开的情况，我们可以使用函数`eval`.

### origin函数

`origin`函数不像是其它大部分函数一样，他不是计算变量的值，而是告诉变量的一些信息。

```Makefile
$(origin variable)
```

注意，`variable`是变量的名字，而不是变量的解引用。

返回值如下

* `undefined`变量未定义
* `default`变量有默认定义，也就是虽然变量在Makefile和环境中未定义，但是它有默认的值，比如`CC`变量通常是系统默认的C编译器。
* `environment`变量是从环境中定义的。
* `environment override`变量是从环境中定义的，而且它覆盖了Makefile里的变量定义。（通过使用命令`-e`.
* `file`变量是在Makefile中定义的。
* `command line`变量是在命令行中定义的。
* `override`变量是用`override`指令定义的。
* `automatic`变量是自动化变量。

```Makefile
ifdef bletch
ifeq "$(origin bletch)" "environment"
bletch = barf, gag, etc.
endif
endif
```

如果变量`bletch`是在环境中定义的，那么就重新定义这个变量。

### 控制Make函数

以下函数控制make的运行，它们用于打印Makefile的信息或者是直接中断make运行。

#### $(error text…)

立刻中断make并输出错误信息`text`.

```Makefile
ifdef ERROR1
$(error error is $(ERROR1))
endif
```

#### $(warning text…)

输出警告信息`text`，这个函数的返回值是空字符串。

#### $(info text…)

这个函数打印`text`信息，返回值是空字符串。

### shell函数

不同于其他所有函数，`shell`函数是与`shell`交互的函数。这意味着它接受`shell`命令作为参数，并且将`shell`的输出作为返回值，同时设置一个特殊的变量`.SHELLSTATUS`作为`shell`退出的状态。

```Makefile
contents := $(shell cat foo)
```

`contents`就包含文件`foo`的内容，但是把换行符变为了空格。

```Makefile
files := $(shell echo *.c)
```

`files`包含了文件目录所有的`.c`文件文件名，以空格分隔。

```Makefile
export HI = $(shell echo hi)
all: ; @echo $$HI
```

所有`export`变量也会传递到`shell`函数开启的`shell`终端中，所以上述命令会打印`hi`.

## 如何运行make

一个makefile会告诉make如何编译或者更新程序。如果我们不指定命令行参数运行`make`，那么make就会默认第一个规则目标为目标。我们也可以指定特殊的命令行参数，使得make的行为不同，比如只是列出过时的文件而不是尝试重建它，或者是指定别的目标。

* GNU 官方文档[GNU make](https://www.gnu.org/software/make/manual/make.html#Running)

### make返回值

* `0`意味着make成功运行
* `2`意味着make运行遇上了错误
* `1`意味着使用`-q`后，make认为一些文件过时(out of date)了。

### 用于指定Makefile的命令行参数

使用`-f`或者是`--file`就可以指定要读取的Makefile，比如`make -f altmake`就会运行`altmake`作为Makefile。
要是使用了`-f`参数好几次，那么所有指定的Makefile文件都会读取。
如果没有使用参数`-f`，那么make默认按顺序搜索`GNUmakefile`,`makefile`,`Makefile`存在的文件，都没有就报错。

### 用于指定目标的参数

目标就是make应该创建或者是更新的最终目标，别的目标只有是这个最终目标的依赖（或者是依赖的依赖）时才会更新。
默认目标就是Makefile的第一个目标。
我们可以直接通过命令行指定`make targetname`就可以指定特定的make目标。

### 不执行配方的命令行参数

可以告诉make不要直接执行配方更新目标，而是做其他的工作。

* `-n` `--just-print` `--dry-run` `--recon`这些命令行参数告诉make不执行配方，而是转而打印过时的目标名字。注意，一些配方也还是会被执行，比如`$(MAKE)`,以及更新Makefile的配方。
* `-q` `--question` 询问是否要执行配方
* `-t` `--touch`不执行配方，但是修改文件的更新日期使得它变为及时(up to date).

## 隐式规则

有些重建规则的方法是十分常见的，比如把每个`.c`文件编译为`.o`文件。
make会根据文件的后缀名自动地寻找隐式规则。
隐式规则告诉make默认的重建配方，而我们不必要真正的指定显式规则。

### 使用隐式规则

为了让`make`为特定目标使用隐式规则，我们就不能为这个特定目标创建显式规则配方，要不就是写**没有**实际配方的显式规则，要不就是直接不写显式规则。

```Makefile
foo : foo.o bar.o
        cc -o foo foo.o bar.o $(CFLAGS) $(LDFLAGS)
```

因为我们提到了`foo.o`但是没有指定显式规则，`make`就会自动地寻找隐式规则，无论`foo`是否存在。
要是找到了隐式规则，隐式规则就会提供对应的依赖(`foo.c`)和配方.
每个隐式规则实际上是和模式规则差不多，都是有目标的模式和依赖的模式。有许多的隐式规则都有相同的目标的模式，比如，大量的隐式规则生成`.o`目标，一个是C编译器把`.c`编译为`.o`、一个是帕斯卡编译器把`.p`编译为`.o`等等。真正应用的隐式规则取决于依赖是否**存在**或者是否可以被**生成**。所以，要是文件目录中存在`foo.c`就会应用C编译器，要是文件目录中存在`foo.p`就会应用帕斯卡编译器。
一个依赖是否可以被生成取决于是否显式地提到了这个文件作为目标或者是依赖，又或者是存在另一个隐式规则可以递归地创建它（隐式规则链）。
如果我们确实不需要make对**特定**目标应用隐式规则，我们可以给目标定义一个**空配方**，空配方和没有配方是不一样的概念。

```Makefile
foo.o: foo.p ;
```

分号后边跟的就是第一条配方，它是一个空配方。

### 模式规则(pattern rules)

我们可以通过定义模式规则来定义隐式规则。模式规则有点像普通规则，但是它包含通配符`%`,文件就会进行模式匹配，匹配写下的模式规则，`%`就是匹配所有非空的字符串，并作为词干被提取出来，和依赖里`%`替换。

```Makefile
%.o : %.c
  recipes
```

上面的模式规则就会告诉make如何从`.c`文件生成`.o`文件。

注意，通配符`%`的展开发生在所有的变量和函数展开**之后**。

注意，模式规则并不强行要求依赖包含`%`，不包含`%`的依赖就是会应用在匹配的**所有**文件的依赖，甚至**不要求**模式规则包含依赖。

要是多个文件模式规则都匹配同一个目标，那么make会选择**最优**的一个。

要是在模式规则中定义超过一个的目标，那么make就会认为这是一个**组目标**。

#### 模式规则实例

接下来讲解规则模式的实例。

```Makefile
%.o : %.c
  $(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
```

定义了一个模式规则，这个模式规则告诉了make如何从`.c`文件**生成**`.o`文件。配方中还使用了自动化变量，展开为依赖和目标的文件名。

#### 自动化变量

使用自动化变量我们可以指定模式规则的目标和依赖的文件名，由于自动化变量是在配方里的，也就是说自动化变量的展开是模式规则匹配完毕相应的文件之后，所以自动化变量就可以对于不同的目标自动替换不同的文件名，十分方便。
通常，在依赖里使用自动化变量是没有意义的，因为此时自动化变量还没有被赋值，但是可以使用特殊的GNU make特性——**二次扩展**来达到想要的目标。

自动化变量列表如下。

为了方便，本文下面使用的例子为

```Makefile
%.o : %.c %.h all_match1 duplicate_match duplicate_match
  $(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
```

并以其匹配了`bar.o : bar.c bar.h all_match1 duplicate_match duplicate_match`为例，同时假定`bar.h`比`bar.o`要新。

* `$@`
  匹配目标的文件名，也就是`bar.o`。
* `$%`
  当目标是存档成员时，匹配目标成员名，较少用。
* `$<`
  匹配第一个依赖文件名，也就是`bar.c`,要是目标是匹配隐式规则，那么这个就会匹配第一个加入到隐式规则的依赖。
* `$?`
  匹配所有比目标**新**的依赖，使用空格分隔，也就是`bar.h`。要是目标文件不存在，那么所有的依赖文件都会被包含。`$?`甚至在显式规则里也很有用，要是我们想要对所有比目标新的依赖做特定的操作的话。
* `$^`
  匹配所有依赖的文件名，使用空格分隔，**不包括**所有仅顺序依赖(order-only prerequisites).同时，将重复的依赖去除，也就是每个依赖只在`$^`中出现一次。也就是`bar.c bar.h all_match1 duplicate_match`.
* `$+`
  和`$^`类似，匹配所有依赖文件名，使用空格分隔，**不包括**仅顺序依赖，**不去除**重复的依赖，也就是`bar.c bar.h all_match1 duplicate_match duplicate_match`.
* `$|`
  匹配所有仅顺序依赖，使用空格分隔。
* `$*`
  匹配模式匹配的词干，但对于具有文件目录的目标和各种规则中有细微的不同。
  对于没有文件目录的目标，匹配模式匹配的词干，也就是`bar`.
  对于有文件目录的目标，匹配模式匹配的词干，比如`dir/a.foo.b`,匹配模式是`a.%.b`，词干就是`dir/foo`.
  对于静态模式规则，词干就是匹配的`%`.
* `$(@D)`
  目标的文件目录，去除了最后一个斜杠。比如`dir/foo.o`，那么`$(@D)`就是`dir`.要是没有目录，这个自动化变量的值就是`.`.
* `$(@F)`
  就是去除了文件路径的目标名字，等同于`$(notdir $@)`.

#### 模式匹配具体细节

一个目标模式就是包含统配符号`%`以及前后缀，前后缀都可以为空。只有当文件名以给定前缀开始和给定后缀结束才会匹配。此时，在前后缀之间的文本就叫做词干(stem).之后模式规则依赖中的通配符`%`就被词干所替换，成为真正的文件名。
要是目标模式中**不包含**斜杠`/`，那么在匹配前，文件名中的目录部分会被**去掉**，之后再进行前后缀匹配，要是匹配成功了，被去除的目录部分就会**加在**包含`%`的规则依赖的前面。make去除目录部分的用意是，尽可能地找到一个符合的隐式规则。所以，`e%t`匹配到文件`src/eat`,词干是`src/a`。当把模式依赖转换为文件名时，词干的目录就会加在前面，剩下的词干与`%`替换。比如若依赖为`c%r`,真正的文件为`src/car`.
当然，要搜索到不同文件夹的文件，我们需要指定`VPATH`变量，告诉make搜索的文件夹,或者在目标中显式指定文件目录。
一个模式规则可以被用于构建一个给定的文件**当且仅当**模式匹配成功**且**所有的依赖存在或是可以被构建。我们写下的模式规则比make自带的隐式规则优先级要高。但是，一个规则不需要进行隐式规则链的通常比那些依赖必须使用隐式规则链构建的优先级高。
若是有两个规则优先级相同，make就会选择最短的词干,若还是相同，那么make就会选择第一个在makefile中找到的规则。

```Makefile
%.o: %.c
        $(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.f
        $(COMPILE.F) $(OUTPUT_OPTION) $<

lib/%.o: lib/%.c
        $(CC) -fPIC -c $(CFLAGS) $(CPPFLAGS) $< -o $@
```

假定make需要构建`bar.o`，此时`bar.c`和`bar.f`都存在，此时make会选择第一个规则，编译`bar.c`为`bar.o`。但要是`bar.c`不存在,`bar.f`存在，make会选择第二个规则，编译`bar.f`为`bar.o`.
假定make需要构建`lib/bar.o`,此时`lib/bar.c`和`lib/bar.f`都存在，make会选择第三个规则，因为它的词干最少。但若是`lib/bar.c`不存在，make只能退而选择第二个规则。

#### 匹配所有文件的模式规则

只有通配符`%`没有前后缀的模式规则，会匹配所有的文件名。这个就是匹配所有文件的模式规则(match-anyting rules).这个规则非常有用，但是会减慢make的运行。
假定makefile提到了`foo.c`，由于这个规则的存在，make就不得不考虑链接`foo.c.o`生成这个可执行文件`foo.c`的可能性。
我们都知道这个可能性是离谱的，最终make也会拒绝这个，因为不存在`foo.c.o`。但是考虑这些可能性确实大大加长了make的运行时间。
为了加快速度，我们需要给匹配所有文件的模式规则加上限制。
第一个选择是，把匹配所有规则的模式规则定义为终极，也就是使用双冒号`::`，这样make就不会考虑它的依赖是否可以被其他的隐式规则所创建了。

#### 取消隐式规则

如果我们想对一类文件取消对应的隐式规则，我们直接定义一个新的模式规则即可.

```Makefile
%.o : %.s
```

通过定义了一个新的模式规则同时没有指定配方,我们可以取消`.o`文件的隐式规则。

假设我们使用

```Makefile
hello : hello.o
	$(CC) hello.o -o hello

%.o : %.c 
```

定义了一个新的模式规则且**没有**配方，那么我们就取消了相应隐式规则，此时我们运行`make`,报的错为

```shell
make: *** No rule to make target 'hello.o', needed by 'hello'.  Stop.
```

这是因为这个模式规则取消了隐式规则，但由于是空配方，在模式搜索算法中被去除了，所以`make`报找不到规则的错误。

假如我们定义的是

```Makefile
hello : hello.o
	$(CC) hello.o -o hello

%.o : %.c ;
```

定义了一个新的模式规则且是**空**配方，我们也取消了相应的隐式规则，此时我们运行`make`,报的错为

```shell
cc hello.o -o hello
cc: error: hello.o: No such file or directory
cc: fatal error: no input files
compilation terminated.
make: *** [Makefile:3: hello] Error 1
```

此时的错误便是在运行`hello:hello.o`的配方的错误了，`cc`发现没有对应的`hello.o`的文件而出错。说明我们规则模式`%.o:%.c ;`成功运行。

#### 定义最终手段默认规则

定义一个终极的匹配所有文件的模式规则，这个规则没有依赖。
这个模式规则匹配所有目标，所以这是一个用于所有没有配方的目标和依赖的最终手段。

```Makefile
% ::
        touch $@
```

这个规则就是把所有没有配方的目标和依赖创建成文件。

#### 隐式规则搜索算法

make为目标`t`搜索隐式规则的流程如下。

1. 将`t`分为目录部分，叫做`d`,和剩余部分，叫做`n`。若`t`是`src/foo.o`,`d`就是`src/`，`n`就是`foo.o`.
2. 寻找所有匹配`t`或者是`n`的模式规则。要是待测试的模式规则包含了斜杠`/`，那么这个模式规则和`t`做匹配；否则和`n`做匹配，将所有找到的模式规则存入make内部的**列表**中。
3. 要是上一步中匹配的规则中，有不是匹配所有文件的模式规则，或者`t`是一个隐式规则的依赖，那么从找到的模式规则列表中去除掉所有不是终极(non-terminal)的匹配所有规则.
4. 从找到的模式规则列表中去除所有**没有**配方的模式规则，此时**没有**配方的模式规则已经**取消**了对应的隐式规则，所以将这中**没有**配方的模式规则去除了，也不会匹配相应的隐式规则。
5. 对于列表中的每个模式规则
   1. 计算词干`s`,计算方法参考上文。
   2. 计算实际依赖的文件名，将`%`替换为词干非目录部分。要是要是模式规则不包含斜杠`/`,将`d`应用在依赖文件名前面。注意，对于没有`%`的依赖，make不会在它前面加上`d`.
   3. 检查每个依赖文件是否存在或者应该存在.要是一个文件名在Makfile作为目标出现或者是它是目标`t`的显式依赖，那么这个文件就是应该存在。
   4. 要是所有的依赖都存在或者是应该存在，那么这个模式规则就通过检验。
6. 要是没有模式规则通过检验，进行更加深入的检验，对于列表中的每个模式规则，
   1. 要是规则是终极的，跳过到下一条规则。
   2. 计算依赖名字。
   3. 检查每个依赖文件是否存在或者应该存在
   4. 对于每个不存在的依赖，递归地查找是否有隐式规则可以构建它。
   5. 要是所有的依赖都存在或者是应该存在，或者可以被隐式规则构建，那么这个规则通过检验。
7. 要是还是没有找到模式规则，修改应该存在的定义：要是文件名作为目标或者是**任何**目标的显式依赖，那么这个文件应该存在。
8. 要是还是没有规则应用，那么默认规则就会被使用，否则报错。
