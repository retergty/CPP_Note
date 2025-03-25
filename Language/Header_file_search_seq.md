# 头文件搜索路径

## 双引号搜索路径

```CPP
#include "head.h"
```

1. 搜索当前目录
2. 搜索`-I`指定的目录
3. 搜索`gcc`环境变量`CPLUS_INCLUDE_PATH`指定的目录
4. 搜索`gcc`内定目录

## 尖括号搜索路径

```CPP
#include <head.h>
```

1. 搜索`-I`指定的目录
2. 搜索`gcc`环境变量`CPLUS_INCLUDE_PATH`指定的目录
3. 搜索`gcc`内定目录

## 解释

### 当前目录

当前目录指的是**正在处理**的文件所在的目录，对于头文件嵌套，当前目录指的是被嵌套的头文件所在的目录.

### 内定目录

内定目录取决于编译器实现。可以通过以下命令显示

```shell
`gcc -print-prog-name=cc1plus` -v
```

通用的目录是

```shell
/usr/local/include
/usr/include
```
