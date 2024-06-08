# get_property

得到指定的属性值

参考文档

* [get_property](https://cmake.org/cmake/help/latest/command/get_property.html)

## 命令格式

```CMake
get_property(<variable>
             <GLOBAL             |
              DIRECTORY [<dir>]  |
              TARGET    <target> |
              SOURCE    <source>
                        [DIRECTORY <dir> | TARGET_DIRECTORY <target>] |
              INSTALL   <file>   |
              TEST      <test>
                        [DIRECTORY <dir>] |
              CACHE     <entry>  |
              VARIABLE           >
             PROPERTY <name>
             [SET | DEFINED | BRIEF_DOCS | FULL_DOCS])
```

## 详细描述

* 指定`DEFINED`关键字后，`resultVar`会是一个布尔值表示`propertyName`属性是否定义。
* 指定`SET`关键字后，`resultVar`会是一个布尔值表示`propertyName`属性是否被设置了值。
* 指定`BRIEF_DOCS`关键字后，`resultVar`会返回该属性简要文字描述。
* 指定`FULL_DOCS`关键字后，`resultVar`会返回该属性完全文字描述。
