# mark_as_advanced

把缓存变量标记为进阶

参考文档

* [mark_as_advanced](https://cmake.org/cmake/help/latest/command/mark_as_advanced.html)

## 命令格式

```CMake
mark_as_advanced([CLEAR|FORCE] <var1> ...)
```

## 详细描述

把缓存变量`var`标记为进阶，这个影响CMake GUI显示这个变量的方法，**只有**勾选了显示进阶这个选项，被标记为进阶的缓存变量才会显示。

`CLEAR`表示把进阶的缓存变量变回非进阶。

`FORCE`表示总是把缓存变量标记为进阶。

如果上面两个选项都没有指定，那么第一次设置缓存变量的进阶状态时，就会设置为进阶。但是，如果这个缓存变量早已有了进阶或者是非进阶的状态，这个命令**什么也不做**。
