# CMAKE_CONFIGURATION_TYPES

参考文档

* [CMAKE_CONFIGURATION_TYPES](https://cmake.org/cmake/help/latest/variable/CMAKE_CONFIGURATION_TYPES.html#variable:CMAKE_CONFIGURATION_TYPES)

这是一个缓存变量，对于多配置生成器，描述了可用的构建类型的**列表**，常见的值有`Debug`,`Release`,`RelWithDebInfo`,`MinSizeRel`,但用户也有可能定义自己的值。

当工程第一次一个构建目录里开始构建时，这个变量在命令`project()`或`enable_language()`调用后就会被初始化，如果定义了环境变量`CMAKE_CONFIGURATION_TYPES`,那么就会使用这个值。否则，对于单配置生成器，这个变量通常为空；对于多配置生成器，这个变量有类似于`Debug;Release;RelWithDebInfo;MinSizeRel`的值。

注意，我们应该避免设置`CMAKE_CONFIGURATION_TYPES`要是这个变量为空，这表示目前是单配置生成器。很多项目也是通过测试`CMAKE_CONFIGURATION_TYPES`是否为空来判断目前是否为单配置生成器。
