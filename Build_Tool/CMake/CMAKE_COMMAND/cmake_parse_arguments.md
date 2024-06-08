# cmake_parse_arguments

可以处理带有关键字的输入

参考文档

* [cmake_parse_arguments](https://cmake.org/cmake/help/latest/command/cmake_parse_arguments.html)

## 命令格式

```CMake
cmake_parse_arguments(<prefix> <options> <one_value_keywords>
                      <multi_value_keywords> <args>...)

cmake_parse_arguments(PARSE_ARGV <N> <prefix> <options>
                      <one_value_keywords> <multi_value_keywords>)
```

## 详细描述

这个命令在CMake之前的版本需要`include(CMakeParseArguments)`才可使用，但是在新版本可以直接使用。

这个命令用于函数或者宏中，用来处理输入参数，给参数分类并分别保存到一系列的变量中。

两个命令格式的区别在于，第一个命令格式是从`<args>...`获取输入参数的，第二个命令是从`ARGV`变量获取参数的。

`<options>`，`<one_value_keywords>`,`<multi_value_keywords>`三个参数是用来定义关键字的，三个参数的不同之处在于，关键字后面是否还有其它的值（没有，一个，多个），CMake把输入与这三个参数中的值对比，并保存到一系列变量中。

`<options>`包含了后面没有值的关键字列表，若是关键字出现在输入参数，则给对应的变量赋值`TRUE`,否则`FALSE`

`<one_value_keywords>`包含了后面有一个值的关键字列表,若是关键字出现在输入参数，则给对应的变量赋值这个值，否则是未定义。

`<multi_value_keywords>`包含了后面有多个值的关键字列表,若是关键字出现在输入参数，则给对应的变量赋值这个值，否则是未定义。

`<prefix>`就是保存在的变量的前缀，保存的变量就形如`<prefix>_<keyword>`前缀加上对应的关键字名字。

没有被处理的参数会进入`<prefix>__UNPARSED_ARGUMENTS`比如在后面没有值的关键字后却附上了值，那这个值就会进入`<prefix>__UNPARSED_ARGUMENTS`.

没有在输入参数中出现的关键字就会进入`<prefix>_KEYWORDS_MISSING_VALUES`.

## 例子

```CMake
macro(my_install)
    set(options OPTIONAL FAST)
    set(oneValueArgs DESTINATION RENAME)
    set(multiValueArgs TARGETS CONFIGURATIONS)
    cmake_parse_arguments(MY_INSTALL "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

    # ...
```

如下使用宏

```CMake
my_install(TARGETS foo bar DESTINATION bin OPTIONAL blub CONFIGURATIONS)
```

结果如下

```CMake
MY_INSTALL_OPTIONAL = TRUE
MY_INSTALL_FAST = FALSE # was not used in call to my_install
MY_INSTALL_DESTINATION = "bin"
MY_INSTALL_RENAME <UNDEFINED> # was not used
MY_INSTALL_TARGETS = "foo;bar"
MY_INSTALL_CONFIGURATIONS <UNDEFINED> # was not used
MY_INSTALL_UNPARSED_ARGUMENTS = "blub" # nothing expected after "OPTIONAL"
MY_INSTALL_KEYWORDS_MISSING_VALUES = "CONFIGURATIONS"
         # No value for "CONFIGURATIONS" given
```
