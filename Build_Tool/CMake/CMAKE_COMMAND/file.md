# file

操作文件的命令

参考文档

* [file](https://cmake.org/cmake/help/latest/command/file.html)

## 命令格式

```CMake
Reading
  file(READ <filename> <out-var> [...])
  file(STRINGS <filename> <out-var> [...])
  file(<HASH> <filename> <out-var>)
  file(TIMESTAMP <filename> <out-var> [...])
  file(GET_RUNTIME_DEPENDENCIES [...])

Writing
  file({WRITE | APPEND} <filename> <content>...)
  file({TOUCH | TOUCH_NOCREATE} <file>...)
  file(GENERATE OUTPUT <output-file> [...])
  file(CONFIGURE OUTPUT <output-file> CONTENT <content> [...])

Filesystem
  file({GLOB | GLOB_RECURSE} <out-var> [...] <globbing-expr>...)
  file(MAKE_DIRECTORY <directories>...)
  file({REMOVE | REMOVE_RECURSE } <files>...)
  file(RENAME <oldname> <newname> [...])
  file(COPY_FILE <oldname> <newname> [...])
  file({COPY | INSTALL} <file>... DESTINATION <dir> [...])
  file(SIZE <filename> <out-var>)
  file(READ_SYMLINK <linkname> <out-var>)
  file(CREATE_LINK <original> <linkname> [...])
  file(CHMOD <files>... <directories>... PERMISSIONS <permissions>... [...])
  file(CHMOD_RECURSE <files>... <directories>... PERMISSIONS <permissions>... [...])

Path Conversion
  file(REAL_PATH <path> <out-var> [BASE_DIRECTORY <dir>] [EXPAND_TILDE])
  file(RELATIVE_PATH <out-var> <directory> <file>)
  file({TO_CMAKE_PATH | TO_NATIVE_PATH} <path> <out-var>)

Transfer
  file(DOWNLOAD <url> [<file>] [...])
  file(UPLOAD <file> <url> [...])

Locking
  file(LOCK <path> [...])

Archiving
  file(ARCHIVE_CREATE OUTPUT <archive> PATHS <paths>... [...])
  file(ARCHIVE_EXTRACT INPUT <archive> [...])
```

## 详细描述

### 读取文件

#### `file(READ <filename> <variable> [OFFSET <offset>] [LIMIT <max-in>] [HEX])`

读取文件`filename`的内容存储在`variable`变量。可以从`offset`开始读取`max-in`字节。

### 写文件

#### `file(WRITE <filename> <content>...)`

#### `file(APPEND <filename> <content>...)`

把`content`内容写入/添加到文件`filename`.

#### `file(TOUCH <files>...)`

如果这个文件`files`不存在，创建一个空的文件`files`.如果这个文件已存在，它的`access`,`modification`时间会被更新。

对于一个已经存在的文件，它的内容不会改变。

#### `file(GENERATE [...])`

为当前`CMake`生成器支持的每个构建配置生成输出文件,这个模式支持生成期表达式。

```CMake
file(GENERATE OUTPUT <output-file>
     <INPUT <input-file>|CONTENT <content>>
     [CONDITION <expression>] [TARGET <target>]
     [NO_SOURCE_PERMISSIONS | USE_SOURCE_PERMISSIONS |
      FILE_PERMISSIONS <permissions>...]
     [NEWLINE_STYLE [UNIX|DOS|WIN32|LF|CRLF] ])
```

`CONDITION`如果为真，则产生输出文件。

`CONTENT`使用`content`作为输入。

`OUTPUT`声明产生的文件名，相对路径相对于`CMAKE_CURRENT_BINARY_DIR`.

`TARGET`声明当分析生成期表达式时使用的目标名，比如`$<COMPILE_FEATURES:...>`

注意，这个模式不会在`configure`阶段产生输出文件。

### 文件系统操作

#### `file(GLOB <variable> [LIST_DIRECTORIES true|false] [RELATIVE <path>] [CONFIGURE_DEPENDS] <globbing-expressions>...)`

#### `file(GLOB_RECURSE <variable> [FOLLOW_SYMLINKS] [LIST_DIRECTORIES true|false] [RELATIVE <path>] [CONFIGURE_DEPENDS] <globbing-expressions>...)`

匹配一系列的文件，这些文件匹配`globbing-expressions`并**作为列表**存储到`variable`.匹配模式`globbing-expressions`和正则表达式相像，但是更加简单。

搜索文件名大小写不敏感。

`CONFIGURE_DEPENDS`表示CMake会在构建系统中加入逻辑判断，判断是否需要重新构建系统，但是这个标志不稳定。

`globbing-expressions`例子有，`*.cxx`,`*.vt?`.

`GLOB_RECURSE`模式会搜索指定目录以及所有子目录。

`LIST_DIRECTORIES`默认情况下，`GLOB_RECURSE`忽略目录，可以使用这个参数保留目录。

`RELATIVE`指定返回的路径是相对于`path`的路径。不指明这个参数则是绝对路径。

#### `file(MAKE_DIRECTORY <directories>...)`

创建文件目录

#### `file(REMOVE <files>...)`

#### `file(REMOVE_RECURSE <files>...)`

删除`file`,`REMOVE_RECURSE`会删除指定的文件目录和文件名。相对目录相对于当前源文件目录。
