# add_dependencies

添加目标间的依赖

参考文档

* [add_dependencies](https://cmake.org/cmake/help/latest/command/add_dependencies.html)

## 命令格式

```CMake
add_dependencies(<target> [<target-dependency>]...)
```

## 详细描述

使得目标`target`依赖于目标`target-dependency`,保证`target-dependency`在`target`前构建。也就是，保证当`target`需要构建时，检查是否`target-dependency`需要重新构建，如果需要，先构建`target-dependency`.
