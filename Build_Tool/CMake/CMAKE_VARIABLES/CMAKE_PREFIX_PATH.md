# CMAKE_PREFIX_PATH

参考文档

* [CMAKE_PREFIX_PATH](https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html#variable:CMAKE_PREFIX_PATH)

`;`分隔的列表，表示`find_package`，`find_program()`,`find_library()`,`find_file()`,`find_path()`的搜索路径。每个文件会搜索这个路径下对应的子文件夹。

还有一个与之对应的较低优先级的同名环境变量`CMAKE_PREFIX_PATH`.
