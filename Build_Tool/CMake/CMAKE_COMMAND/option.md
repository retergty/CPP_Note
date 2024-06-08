# option

设置布尔类型的缓存变量

参考文档

* [option](https://cmake.org/cmake/help/latest/command/option.html)

## 命令格式

```CMake
option(<optVar> "<helpString>" [initialValue])
```

## 详细描述

设置布尔类型的缓存变量

要是`initialValue`被省略，则默认值是`OFF`.`option`命令不支持`FORCE`关键字。
