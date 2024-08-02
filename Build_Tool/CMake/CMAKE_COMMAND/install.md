# install

指定在安装时运行的规则

参考文档

* [install](https://cmake.org/cmake/help/latest/command/install.html)

## 命令格式

```CMake
install(TARGETS <target>... [...])
install(IMPORTED_RUNTIME_ARTIFACTS <target>... [...])
install({FILES | PROGRAMS} <file>... [...])
install(DIRECTORY <dir>... [...])
install(SCRIPT <file> [...])
install(CODE <code> [...])
install(EXPORT <export-name> [...])
install(RUNTIME_DEPENDENCY_SET <set-name> [...])
```

## 详细描述

这个命令给工程生成一个安装规则(rule)。这些安装规则会在安装时顺序执行。

`3.14`版本之后，使用`add_subdirectory()`创建的子文件夹中的安装规则会与父文件夹的安装规则交错，按照写下命令的顺序。

## 签名

`CMake`给这个命令提供了多个签名。，不同的签名实现了不同的功能，比如安装目标，安装文件等。

### 适用于所有签名的参数

#### `DESTINATION <dir>`

声明文件需要安装到的目录`dir`,`dir`应该是相对路径，可以接受绝对路径，但是不建议。

相对路径被解释为相对于`CMAKE_INSTALL_PREFIX`变量的值。可以使用`CMAKE_INSTALL_PREFIX`变量的`DESTDIR`机制在安装时重新定位前缀。

#### `PERMISSIONS <permission>...`

指明安装后的文件具有的权限，`permission`可以是`OWNER_READ`,`OWNER_WRITE`,`OWNER_EXECUTE`, `GROUP_READ`,`GROUP_WRITE`,`GROUP_EXECUTE`,`WORLD_READ`,`WORLD_WRITE`,`WORLD_EXECUTE`, `SETUID`,`SETGID`.在特定平台上没有意义的权限会被忽略。

如果多次使用了这个参数，那么它的权限会叠加。

#### `CONFIGURATIONS <config>...`

声明指定安装规则适用的构建配置列表，比如`Debug,Release`.

如果多次使用了这个参数，那么它会叠加。

#### `COMPONENT <component>`

指定与安装规则关联的安装组件名称，例如`Runtime`或`Development`。在特定于组件的安装期间，仅执行与给定组件名称对应的安装规则。而在完整安装过程中，除非标有`EXCLUDE_FROM_ALL`，否则将安装所有组件。

如果未指定则默认值是`Unspecified`.可以通过`CMAKE_INSTALL_DEFAULT_COMPONENT_NAME`变量修改。

#### `EXCLUDE_FROM_ALL`

指定该文件从完整安装中排除，并且仅作为特定于组件的安装的一部分进行安装.

#### `RENAME <name>`

为安装的文件指定一个可能与原始文件不同的名称。仅当通过命令安装单个文件时才允许重命名。

#### `OPTIONAL`

指定要安装的文件不存在也不报错。

### `install(TARGETS <target>... [...])`

安装目标的输出文件上。

```cmake
install(TARGETS <target>... [EXPORT <export-name>]
        [RUNTIME_DEPENDENCIES <arg>...|RUNTIME_DEPENDENCY_SET <set-name>]
        [<artifact-option>...]
        [<artifact-kind> <artifact-option>...]...
        [INCLUDES DESTINATION [<dir> ...]]
        )
```

`<artifact-option>`可以是

```cmake
[DESTINATION <dir>]
[PERMISSIONS <permission>...]
[CONFIGURATIONS <config>...]
[COMPONENT <component>]
[NAMELINK_COMPONENT <component>]
[OPTIONAL] [EXCLUDE_FROM_ALL]
[NAMELINK_ONLY|NAMELINK_SKIP]
```

第一个`<artifact-option>...`应用在所有的目标输出文件上。除非后面的参数直接指定了这个文件组。

每个`<artifact-kind> <artifact-option>...`都应用在输出文件的特定类型上面,`<artifact-kind>`类型如下：

* `ARCHIVE`
  
  这种类型的输出文件包括

  * 静态库(Static libraries),除了macOS标注为`FRAMEWORK`的。
  * `DLL`导入库(DLL import libraries),这是在Windows系统上才有的概念
  
* `LIBRARY`

  这种类型的输出文件包括

  * 共享库(Shared libraries),除了`DLL`或`FRAMEWORK`.

* `RUNTIME`

  这种类型的输出文件包括

  * 可执行文件(Executables)
  * `DLL`,这是在Windows系统上才有的概念，注意`DLL`导入库是`ARCHIVE`类型。

* `PUBLIC_HEADER`

  与库关联的任何`PUBLIC_HEADER`文件都安装在`PUBLIC_HEADER`参数指定的位置中。

* `PRIVATE_HEADER`

  和`PUBLIC_HEADER`类似。

对于常规的可执行文件，共享库，静态库，`DESTINATION`参数不是必要的。如果省略掉这个参数，那么会从`GNUInstallDirs`中选择默认的路径。但是对于`C++ MODULES`则不能省略。

对于`DLL`平台上的共享库,如果没有指定`RUNTIME`和`ARCHIVE`,则`RUNTIME`和`ARCHIVE`组件都会安装到其默认路径。如果只指定了其中一个，那么只会安装一个，另一个不会安装。

常见的默认安装路径如下

|  **Target Type** | **GNUInstallDirs Variable**  | **Built-In Default**  |
|---|---|---|
| `RUNTIME`  |  `${CMAKE_INSTALL_BINDIR}` | `bin`  |
|  `LIBRARY` | `${CMAKE_INSTALL_LIBDIR}`  | `lib`  |
|  `ARCHIVE` | `${CMAKE_INSTALL_LIBDIR}`  | `lib` |
|  `PRIVATE_HEADER` |`${CMAKE_INSTALL_INCLUDEDIR}`  | `include`  |
|  `PUBLIC_HEADER` | `${CMAKE_INSTALL_INCLUDEDIR}`  | `include`  |
|  `FILE_SET (type HEADERS)` | `${CMAKE_INSTALL_INCLUDEDIR}`  | `include`  |

* `NAMELINK_COMPONENT`

  在某些平台上，共享库使用符号链接，比如

  ```text
  lib<name>.so -> lib<name>.so.1
  ```

  可以方便地指定版本号，如果未指定，则默认是`COMPONENT`.

* `[EXPORT <export-name>]`
  创建一个名为`<export-name>`的导出目标，这个导出目标`IMPORTED targets`会正确地设置它的使用需求，比如`INTERFACE_INCLUDE_DIRECTORIES`,`INTERFACE_COMPILE_DEFINITIONS`等。

  此时，`CMake`还没有实际`install`这个导出目标，之后需要使用`install(EXPORT)`到处这个目标，比如

  ```CMake
  install(EXPORT <export-name>
        FILE <export-name>.cmake
        NAMESPACE MathFunctions::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MathFunctions
  )
  ```

  就会生成一个`<export-name>.cmake`，包含所有导出目标。

### `install(EXPORT <export-name> [...])`

生成配置文件。

```CMake
install(EXPORT <export-name> DESTINATION <dir>
        [NAMESPACE <namespace>] [FILE <name>.cmake]
        [PERMISSIONS <permission>...]
        [CONFIGURATIONS <config>...]
        [CXX_MODULES_DIRECTORY <directory>]
        [EXPORT_LINK_INTERFACE_LIBRARIES]
        [COMPONENT <component>]
        [EXCLUDE_FROM_ALL]
        [EXPORT_PACKAGE_DEPENDENCIES])
install(EXPORT_ANDROID_MK <export-name> DESTINATION <dir> [...])
```

这个命令生成一个`.cmake`文件并安装到对应位置.之后可以通过`find_package`找到这个配置文件并导入，从而导入指定的库文件。

* `EXPORT <export-name>`导出的目标，`<export-name>`就是之前的`install(TARGETS <target> EXPROT <export-name>)`指定的`<export-name>`.
* `NAMESPACE`表示使用库文件时需要附加的名称空间。
* `FILE`表示生成的`.cmake`的名字，默认是`<export-name>.cmake`.

生成的文件类似

```CMake
# Create imported target MathFunctions::MathFunctions
add_library(MathFunctions::MathFunctions STATIC IMPORTED)

set_target_properties(MathFunctions::MathFunctions PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
)
```

我们在使用时便需要

```CMake
target_link_libraries(myexe PRIVATE MathFunctions::MathFunctions)
```
