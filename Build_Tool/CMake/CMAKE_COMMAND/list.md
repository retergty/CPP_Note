# list

处理列表

参考文档

* [list](https://cmake.org/cmake/help/latest/command/list.html)

## 命令格式

```CMake
Reading
  list(LENGTH <list> <out-var>)
  list(GET <list> <element index> [<index> ...] <out-var>)
  list(JOIN <list> <glue> <out-var>)
  list(SUBLIST <list> <begin> <length> <out-var>)

Search
  list(FIND <list> <value> <out-var>)

Modification
  list(APPEND <list> [<element>...])
  list(FILTER <list> {INCLUDE | EXCLUDE} REGEX <regex>)
  list(INSERT <list> <index> [<element>...])
  list(POP_BACK <list> [<out-var>...])
  list(POP_FRONT <list> [<out-var>...])
  list(PREPEND <list> [<element>...])
  list(REMOVE_ITEM <list> <value>...)
  list(REMOVE_AT <list> <index>...)
  list(REMOVE_DUPLICATES <list>)
  list(TRANSFORM <list> <ACTION> [...])

Ordering
  list(REVERSE <list>)
  list(SORT <list> [...])
```

## 详细描述

CMake提供了一系列的操作来处理列表。以下分功能描述。

### 读取数据

#### `list(LENGTH <list> <out-var>)`

取得列表`list`的长度信息,存放在`out-var`里

#### `list(GET <list> <element index> [<index> ...] <out-var>)`

得到列表`list`的第`<element index>`的值，存放在`out-var`里

#### `list(JOIN <list> <glue> <output variable>`

得到使用`glue`分隔的`list`的字符串，存放在`output variable`里

#### `list(SUBLIST <list> <begin> <length> <output variable>)`

得到列表`list`的从`begin`开始，长度为`length`的子列表，存放在`output variable`里

### 搜索

#### `list(FIND <list> <value> <output variable>)`

返回在`list`里找的`value`的下标号，没找到则是`-1`，存放在`output variable`里

### 修改

#### `list(APPEND <list> [<element> ...])`

在`list`后面添加`element`,如果先前`list`未定义，那么就当成空列表，并在后面添加。

#### `list(FILTER <list> <INCLUDE|EXCLUDE> REGEX <regular_expression>)`

保留或者是移去符合正则表达式`regular_expression`的列表项。

#### `list(POP_BACK <list> [<out-var>...])`

从后边移去`out-var...`数量的列表项，存放在`out-var`里

#### `list(POP_FRONT <list> [<out-var>...])`

从前边移去`out-var...`数量的列表项，存放在`out-var`里

#### `list(PREPEND <list> [<element> ...])`

在`list`前面添加`element`,如果先前`list`未定义，那么就当成空列表，并在前面添加。

#### `list(REMOVE_ITEM <list> <value> [<value> ...])`

移去所有在`list`里的`value`表项。

#### `list(REMOVE_AT <list> <index> [<index> ...])`

移去`list`里第`index`表项

#### `list(REMOVE_DUPLICATES <list>)`

移去`list`重复项

### `排序`

#### `list(REVERSE <list>)`

反转`list`

#### `list(SORT <list> [COMPARE <compare>] [CASE <case>] [ORDER <order>])`

使用`COMPARE`选择排序方法，可选的有

* `STRING`按照字母表大小排序
* `FILE_BASENAME`按照文件名方法排序。
* `NATURAL`使用数值方法排序

使用`CASE`选择大小写敏感，可选的有

* `SENSITIVE`大小写敏感
* `INSENSITIVE`大小写不敏感

使用`ORDER`选择正序还是倒序，可选的有

* `ASCENDING`正序
* `DESCENDING`倒序

#### `list(TRANSFORM <list> <ACTION> [<SELECTOR>] [OUTPUT_VARIABLE <output variable>])`

将`ACTION`应用到`list`的每一项。
