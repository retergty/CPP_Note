# string

对字符串进行处理

参考文档

* [string](https://cmake.org/cmake/help/latest/command/string.html)

## 命令格式

```CMake
Search and Replace
  string(FIND <string> <substring> <out-var> [...])
  string(REPLACE <match-string> <replace-string> <out-var> <input>...)
  string(REGEX MATCH <match-regex> <out-var> <input>...)
  string(REGEX MATCHALL <match-regex> <out-var> <input>...)
  string(REGEX REPLACE <match-regex> <replace-expr> <out-var> <input>...)

Manipulation
  string(APPEND <string-var> [<input>...])
  string(PREPEND <string-var> [<input>...])
  string(CONCAT <out-var> [<input>...])
  string(JOIN <glue> <out-var> [<input>...])
  string(TOLOWER <string> <out-var>)
  string(TOUPPER <string> <out-var>)
  string(LENGTH <string> <out-var>)
  string(SUBSTRING <string> <begin> <length> <out-var>)
  string(STRIP <string> <out-var>)
  string(GENEX_STRIP <string> <out-var>)
  string(REPEAT <string> <count> <out-var>)

Comparison
  string(COMPARE <op> <string1> <string2> <out-var>)

Hashing
  string(<HASH> <out-var> <input>)

Generation
  string(ASCII <number>... <out-var>)
  string(HEX <string> <out-var>)
  string(CONFIGURE <string> <out-var> [...])
  string(MAKE_C_IDENTIFIER <string> <out-var>)
  string(RANDOM [<option>...] <out-var>)
  string(TIMESTAMP <out-var> [<format string>] [UTC])
  string(UUID <out-var> ...)

JSON
  string(JSON <out-var> [ERROR_VARIABLE <error-var>]
         {GET | TYPE | LENGTH | REMOVE}
         <json-string> <member|index> [<member|index> ...])
  string(JSON <out-var> [ERROR_VARIABLE <error-var>]
         MEMBER <json-string>
         [<member|index> ...] <index>)
  string(JSON <out-var> [ERROR_VARIABLE <error-var>]
         SET <json-string>
         <member|index> [<member|index> ...] <value>)
  string(JSON <out-var> [ERROR_VARIABLE <error-var>]
         EQUAL <json-string1> <json-string2>)
```

## 详细描述

CMake提供了许多方法处理字符串，以下分别讲述

### 查找与替换

#### `string(FIND <string> <substring> <output_variable> [REVERSE])`

返回在`string`中找到的`substring`第一个位置，存储在`<output_variable>`,`REVERSE`意味着找最后一个位置。

#### `string(REPLACE <match_string> <replace_string> <output_variable> <input> [<input>...])`

替换`input`里所有匹配`match_string`的字符串为`replace_string`，存储在`<output_variable>`。

#### `string(REGEX MATCH <regular_expression> <output_variable> <input> [<input>...])`

正则表达式查找

#### `string(REGEX MATCHALL <regular_expression> <output_variable> <input> [<input>...])`

正则表达式查找全部

#### `string(REGEX REPLACE <regular_expression> <replacement_expression> <output_variable> <input> [<input>...])`

正则表达式替换全部

### 字符串操作

#### `string(APPEND <string_variable> [<input>...])`

添加`input`到字符串`string_variable`后

#### `string(PREPEND <string_variable> [<input>...])`

添加`input`到字符串`string_variable`前

#### `string(CONCAT <output_variable> [<input>...])`

将所有的`input`连接起来，存储在`output_variable`

#### `string(JOIN <glue> <output_variable> [<input>...])`

讲所有的`input`用`glue`连接起来

#### `string(TOLOWER <string> <output_variable>)`

转为小写

#### `string(TOUPPER <string> <output_variable>)`

转为大写

#### `string(LENGTH <string> <output_variable>)`

字符串长度

#### `string(SUBSTRING <string> <begin> <length> <output_variable>)`

返回字符串`string`从`begin`开始长度为`length`的子字符串，存储在`output_variable`.

#### `string(STRIP <string> <output_variable>)`

去除字符串`string`中所有的前导和尾随空格，存储在`output_variable`中

#### `string(GENEX_STRIP <string> <output_variable>)`

去除字符串`string`中所有的生成期表达式，存储在`output_variable`中

#### `string(REPEAT <string> <count> <output_variable>)`

把`string`重复`count`次，存储在`output_variable`

### 字符串比较

#### `string(COMPARE LESS <string1> <string2> <output_variable>)`

#### `string(COMPARE GREATER <string1> <string2> <output_variable>)`

#### `string(COMPARE EQUAL <string1> <string2> <output_variable>`

#### `string(COMPARE NOTEQUAL <string1> <string2> <output_variable>)`

#### `string(COMPARE LESS_EQUAL <string1> <string2> <output_variable>)`

#### `string(COMPARE GREATER_EQUAL <string1> <string2> <output_variable>)`

比较两个字符串并将布尔值存储在`output_variable`中
