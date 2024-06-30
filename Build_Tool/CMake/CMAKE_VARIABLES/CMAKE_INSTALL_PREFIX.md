# CMAKE_INSTALL_PREFIX

参考文档

* [CMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html#variable:CMAKE_INSTALL_PREFIX)

用于`install()`命令的目录。

如果调用`make install`或构建`INSTALL`，则此变量将添加到所有安装目录之前。

这个变量的默认值如下

* 在`Windows`里，是`c:/Program Files/${PROJECT_NAME}`
* 在`Unix`里，是`/usr/local`.

在`Unix`里，可以使用`DESTDIR`机制将整个安装重新定位到暂存区域。

用户也可以在命令行指明`--prefix`选项重新设置变量。

```cmake
cmake --install . --prefix /my/install/prefix
```