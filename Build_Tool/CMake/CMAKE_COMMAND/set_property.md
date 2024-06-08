# set_property

设置属性

参考文档

* [set_property](https://cmake.org/cmake/help/latest/command/set_property.html#set-property)

## 命令格式

```CMake
set_property(<GLOBAL                      |
              DIRECTORY [<dir>]           |
              TARGET    [<target1> ...]   |
              SOURCE    [<src1> ...]
                        [DIRECTORY <dirs> ...]
                        [TARGET_DIRECTORY <targets> ...] |
              INSTALL   [<file1> ...]     |
              TEST      [<test1> ...]
                        [DIRECTORY <dir>] |
              CACHE     [<entry1> ...]    >
             [APPEND] [APPEND_STRING]
             PROPERTY <name> [<value1> ...])
```

## 详细描述

设置属性，第一个参数是属性属于的对象。

`GLOBAL`就是全局属性，不需要额外指定参数。`DIRECTORY`若是不指定`dir`则指定的就是当前的目录。

`PROPERTY`后面就是属性名和要修改的值。

除了修改CMake预定义的属性，我们还可以定义自己的属性，只需要给出想要定义的属性名和设置的值就可以了。

```CMake
set_property(TARGET MyApp1 MyApp2
    PROPERTY MYPROJ_CUSTOM_PROP val1 val2 val3
)
```

上述的命令为`MyApp1`和`MyApp2`定义了`MYPROJ_CUSTOM_PROP`属性，这个属性值为`val1;val2;val3`.

`APPEND`和`APPEND_STRING`可以被用于控制`values`如何加入这个属性中，`APPEND`就是在原属性值后加上分号`;`和`values`.`APPEND_STRING`在原属性值后加上`values`.要是不使用`APPEND`和`APPEND_STRING`，则是替换原来的值。

当使用`CACHE`设置缓存变量的属性时，缓存变量应该已经存在。全局只有一个缓存变量区，所以在子目录下的缓存变量属性修改在父目录也会存在。

缓存变量属性和别的属性不太一样，它不影响缓存变量在构建过程中的行为，只是控制着对应的缓存变量是如何在`CMake GUI`中显示的。

常用的缓存变量属性如下

* `TYPE`属性，这个属性值可以是`BOOL`,`FILEPATH`,`PATH`,`STRING`,`INTERNAL`其中之一，控制着缓存变量在GUI中的显示方法。
* `ADVANCED`属性，这个属性是一个布尔值，通常使用命令`mark_as_advanced()`设置，控制缓存变量是否是高级选项。
* `HELPSTRING`属性，这个属性是一个字符串，通常在使用`set()`设置缓存变量时就顺便设置了，但也可以单独改变。
* `STRINGS`属性，这个属性是一个字符串列表，当缓存变量的类型为`STRING`时，CMake就会搜索这个属性，如果这个属性不为空，这个属性就会被认为是这个缓存变量的合法值，CMake GUI会提供一个下拉菜单。注意，这个属性不会强制这个缓存变量必须满足这些值，只是为了方便GUI工具。
