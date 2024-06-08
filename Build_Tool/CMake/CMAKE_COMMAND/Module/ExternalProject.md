# ExternalProject

创造一个**自定义目标**(custom target)，这个目标是从工程外部下载，更新，配置，构建，测试的。

参考文档

* [ExternalProject](https://cmake.org/cmake/help/latest/module/ExternalProject.html#externalproject)

## ExternalProject_Add

添加一个外部工程

### 命令格式

```CMake
ExternalProject_Add(<name> [<option>...])
```

### 详细描述

#### 目录选项

通常情况下，目录的默认选项已经完全足够，但是，我们也可以通过显式指明目录选项来修改默认行为。

`PREFIX <dir>`外部工程的根目录。

`TMP_DIR <dir>`外部工程用来存储临时文件的目录

`STAMP_DIR <dir>`外部工程用来存储时间戳文件的存储目录，如果没有指定`LOG_DIR`,那么`log`文件也会存储到这里

`LOG_DIR <dir>`外部工程用来存储`log`文件的位置。

`DOWNLOAD_DIR <dir>`外部工程用来从存储url下载的文件的目录

`SOURCE_DIR <dir>`外部工程存储从`url`下载的文件解压后的文件的目录，对于不使用`url`下载的外部工程，这个选项表示外部工程`clone`,`check out`等的目录。如果没有提供下载方法的`option`，这个必须指向一个早已存在的目录，这个目录里有早已解压，或者`git clone`，`git checkout`的内容。

`BINARY_DIR <dir>`外部工程用来存储二进制文件的目录。

`INSTALL_DIR <dir>`外部工程安装的目录，这个选项不会实际配置外部工程去安装到对应的目录，我们还需要给外部工程传递构建变量。

这些目录都可以使用相对目录，相对目录相对于`CMAKE_CURRENT_BINARY_DIR`.

#### 下载步骤选项

如果`SOURCE_DIR <dir>`指向一个非空的目录，那么可以省略下载方法。否则，必须提供一个下载方法或者是自定义`DOWNLOAD_COMMAND`选项。

`DOWNLOAD_COMMAND <cmd>...`提供一个自定义的下载方法选项，运行`cmd`.

##### 从URL下载

`URL <url1> [<url2>...]`外部工程下载文件的`url`,如果指定了多个`url`那么就会顺序执行下载。

`DOWNLOAD_NAME <fname>`把下载文件命名为`fname`，如果没有指定，那么就以`url`最后的部分命名。

##### 从git下载

`GIT_REPOSITORY <url>`指明下载的库`url`。

`GIT_TAG <tag>`下载的git库的分支名或`tag`名，或者是`commit id`.注意分支名与`tag`名应该是远程库的名字，比如`origin/master`.通常，声明一个`commit id`是更好的方法。它既加快了速度，也保证工程跟踪的外部工程不会悄悄地改变。

#### 更新步骤选项

当CMake重新运行时，默认这些外部工程都会检查是否需要更新。但我们也可以通过选项修改。

#### 配置步骤选项

当下载和更新完成后，就可以运行配置任务，默认情况下，外部工程被认为是CMake工程，我们也可以通过选项修改。

`CONFIGURE_COMMAND <cmd>...`默认情况下外部工程会运行`CMake`.我们可以通过这个选项，自定义配置命令。

#### 构建步骤选项

当配置完成后，就会开始构建，默认情况下，外部工程被认为是CMake工程，运行CMake。

`BUILD_COMMAND <cmd>...`我们可以通过这个选项，自定义构建命令。

#### 输出信息选项

选择输出的具体信息，对应的`log`文件会存储在`LOG_DIR`里。

`LOG_DOWNLOAD <bool>`为`ON`时，下载步骤的输出会存储在`log`里。

`LOG_UPDATE <bool>`为`ON`时，更新步骤的输出会存储在`log`里。

`LOG_CONFIGURE <bool>`为`ON`时，配置步骤的输出会存储在`log`里。

`LOG_BUILD <bool>`为`ON`时，构建步骤的输出会存储在`log`里。

#### 目标选项

`DEPENDS <targets>...`设置这个外部模块所依赖的目标，被依赖的目标会在外部目标的步骤运行前更新。

`EXCLUDE_FROM_ALL <bool>`把这个外部模块从默认的`ALL`目标中排除。

## 注意点

注意，外部模块的获取是在`build`阶段，和`custom target`的构建阶段一样。

## 例子

`PX4`代码的`CMakeLists.txt`

```CMake
cmake_minimum_required(VERSION 2.8.4)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929
	SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-src"
	BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND ""
	# Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)
```

在另一个`CMakeLists.txt`中

```CMake
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . RESULT_VARIABLE result1 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
execute_process(COMMAND ${CMAKE_COMMAND} --build . RESULT_VARIABLE result2 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
```

就会在`configure`阶段运行第一个`CMakeLists.txt`.
