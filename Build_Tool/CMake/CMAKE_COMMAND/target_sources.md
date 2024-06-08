# target_sources

给目标添加新的源文件

参考文档

* [target_sources](https://cmake.org/cmake/help/latest/command/target_sources.html#command:target_sources)

## 命令格式

```CMake
target_sources(<target>
  <INTERFACE|PUBLIC|PRIVATE> [items1...]
  [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
```

## 详细描述

给目标`target`添加新的源文件

按照参数`<INTERFACE|PUBLIC|PRIVATE>`的使用，给目标的`SOURCES`、`INTERFACE_SOURCES`属性添加新的表项，这三个参数表述了使用这个`target`的别的目标的使用要求。
