# CMAKE_BUILD_TYPE

参考文档

* [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)

这是一个缓存变量，表示了当前的构建类型，只在单配置生成器(比如Makefile)中有效果，对于多配置生成器(VS Studio)是没有效果的。常见的值有`Debug`,`Release`,`RelWithDebInfo`,`MinSizeRel`,但用户也有可能定义自己的值。

当工程第一次一个构建目录里开始构建时，这个变量在命令`project()`或`enable_language()`调用后就会被初始化，如果定义了环境变量`CMAKE_BUILD_TYPE`,那么就会使用这个值，否则通常就是空字符串。
