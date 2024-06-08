# add_compile_options

添加编译器编译时的参数

参考文档

* [add_compile_options](https://cmake.org/cmake/help/latest/command/add_compile_options.html)

## 命令格式

```CMake
add_compile_options(<option> ...)
```

## 详细描述

添加`<option>`到当前目录的`COMPILE_OPTIONS`属性中，**这个列表**的每一项会直接被传递给编译器作为编译时的参数（不包括链接时），CMake**不做任何处理**（除了删除重复的表项重复以及自动加上转义字符防止转义）。

CMake不做任何处理，甚至不会去除传递给命令的`""`.

```CMakeLists
add_compile_options("-Wall -Werror -Wextra");
```

直接传递给编译器参数`"-Wall -Werror -Wextra"`不去除双引号。

最后传递给编译器的参数由目标自己的`COMPILE_OPTIONS`属性决定，（包括它从别的库中继承的使用要求）。之后，CMake会自动去除重复项。

这个命令支持编译期表达式。
