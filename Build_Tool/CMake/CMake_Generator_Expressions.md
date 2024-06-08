# CMake 生成期表达式

参考文件

* CMake 生成期表达式大全[cmake-generator-expressions](https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html)

有的时候用户需要使用在`generate`阶段才会知道的信息，希望将某些信息的获取延迟到所有`CMakeFileLists.txt`读取完毕后,此时就可以使用生成期表达式。

```CMake
$<Operator:...>
```

## Debug

由于生成期表达式是在`build`阶段才会运行，所以我们不能使用`message()`打印生成期表达式。我们可以加入自定义目标来解决这个问题。

```CMake
add_custom_target(genexdebug COMMAND ${CMAKE_COMMAND} -E echo "$<...>" VERBATIM)
```

我们就可以在命令行输入

```shell
cmake --build ... --target genexdebug
```

就可以打印目标了。

## 简单的布尔逻辑生成期表达式

最简单的格式为

```CMake
$<1:...>
$<0:...>
```

对于`$<1:...>`结果就会被展开为`...`;对于`$<0:...>`结果就会被展开为空字符串.

`0`和`1`可以被替换为任何变量展开`${var}`或者也可以是生成期表达式。但是不同于`if`语句，任何最终不被展开成`0`和`1`的都会报错。

### `$<BOOL:...>`

这个生成期表达式会按照`if`语句的习惯来处理，如果`...`部分被视为真，则该生成期表达式返回`1`；反之，该生成期表达式返回`0`.

### `$<AND:expr[,expr...]>`

### `$<OR:expr[,expr...]>`

### `$<NOT:expr>`

这三个生成期表达式进行逻辑处理，这些`expr`都必须展开为`0`或者`1`.

### `$<IF:expr,val1,val0>`

要是`expr`展开为`1`，则表达式展开为`var1`.

要是`expr`展开为`0`，则表达式展开为`var0`.

同样的，`expr`都必须展开为`0`或者`1`.

### `$<CONFIG:arg>`

要是`arg`为目前选择的`build`类型，则结果展开为`1`，否则展开为`0`.

`arg`是目前构建的类型，一般是`Release`,`Debug`等，对于Visual Studio等多构建类型的IDE来说，构建的类型只有在`generate`阶段才会知道.所以使用生成器表达式是一个更加有鲁棒性的操作。

## 取得目标信息

通常使用生成器表达式取得目标的信息，这是由于随着CMake读取CMakeFileLists.txt进行，目标的信息可能会改变，也只有在`generate`阶段，目标的信息才能真正确定下来.

### `$<TARGET_PROPERTY:target,property>`

取得目标`target`的`property`属性信息。

### `$<TARGET_PROPERTY:property>`

取得**当前**目标的`property`信息。

### `$<TARGET_FILE:tgt>`

取得目标`tgt`的二进制文件的完整路径.

### `$<TARGET_FILE_NAME:tgt>`

取得目标`tgt`的二进制文件的文件名

### `$<TARGET_FILE_DIR:tgt>`

取得目标`tgt`的二进制文件的路径

### `$<TARGET_OBJECTS:tgt>`

取得构建目标`tgt`的`.o`文件名，包含绝对地址。

## 取得通用信息

生成期表达式可以取得项目的通用信息，比如正在使用的编译器，构建目标的平台等

### `$<CONFIG>`

取得当前构建的类型名

### `$<PLATFORM_ID>`

取得当前平台名

### `$<C_COMPILER_VERSION>, $<CXX_COMPILER_VERSION>`

取得当前C/C++编译器的版本。

### `$<COMPILE_LANGUAGE>`

取得当前编译的语言(C/C++/CUDA等).

### `$<COMPILE_LANGUAGE:languages>`

若是当前编译的语言是`languages`则返回`1`,否则返回`0`.通常与布尔生成期表达式结合在一起，比如`$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>`.

## 用于安装或者是导出的生成期表达式

### `$<INSTALL_INTERFACE:...>`

当目标使用`install(EXPORT)`安装时，返回`...`.否则为空。

### `$<BUILD_INTERFACE:...>`

当目标使用`export()`导出或者是这个目标被别的目标所使用时，返回`...`.否则为空

## 特殊用途生成期表达式

这些特殊用途的生成期表达式可以用来产生空格，逗号等，用来分隔。

### `$<COMMA>`

返回逗号

### `$<SEMICOLON>`

返回分号

### `$<LOWER_CASE:…>, $<UPPER_CASE:…>`

将输入`...`变为大写或者是小写。

### `$<JOIN:list,…>`

将列表`list`里的`;`替换为`...`.注意，这个替换生成期表达式必须使用`""`括起来，防止`list`里的`;`被当做分隔符.

```CMake
set(dirs here there) # dirs = here;there

set_target_properties(Foo PROPERTIES
CUSTOM_INC "-I$<JOIN:${dirs}, -I>"
)
```
