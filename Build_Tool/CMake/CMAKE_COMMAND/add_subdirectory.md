# add_subdirectory

加入子文件夹，转而读取子文件夹下的`CMakeFileLists.txt`.

参考文档

* [add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html)

## 命令格式

```CMake
add_subdirectory(source_dir [binary_dir] [EXCLUDE_FROM_ALL] [SYSTEM])
```

## 详细描述

加入子文件夹，转而读取子文件夹下的`CMakeFileList.txt`.

`source_dir`就表示要读取的`CMakeFileLists.txt`所在的文件目录，可以是相对路径，相对于当前的源文件目录。

`binary_dir`就表示要把读取的`CMakeFileLists.txt`所生成的二进制文件的存放位置，可以是相对路径，相对于当前的二进制文件目录。要是`binary_dir`没有出现，那么就使用`source_dir`,只不过是相对的是当前的二进制文件目录。

子文件夹下的`CMakeFileList.txt`会立即开始读取，并创建一个子作用域，读取完毕后，才会继续当前`CMakeFileList.txt`的读取。

`EXCLUDE_FROM_ALL`表示所有在子文件夹下定义的目标都会被排除在`ALL`目标外，我们必须显式编译这些目标才行。
