# CMAKE_SYSROOT

参考文档

* [CMAKE_SYSROOT](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSROOT.html)

设置系统的根目录，这个变量的值会传递给编译器，使用参数`--sysroot`.

对于`gcc`编译器,`gcc --sysroot=dir`指定了逻辑根目录，直接影响了编译器搜索系统库文件和系统头文件的路径。编译器通常会在`/usr/include`和`/usr/lib`中搜索没有显式指明搜索目录的头文件和库文件，添加了参数`--sysroot=dir`就会在`dir/usr/include`和`dir/usr/lib`中搜索。

这个变量也会被用于`find_*`一系列命令，作为搜索路径的前缀。

这个变量在交叉编译环境十分有用，用于避免编译器查找到主机的库文件而引起错误。
