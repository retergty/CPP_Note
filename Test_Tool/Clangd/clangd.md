# clangd

`clangd` 是一个基于`LLVM`的`C/C++`语言服务器，提供智能代码补全、错误检测、代码导航等功能，提升开发效率。

在编辑器中安装并配置好`clangd`后，它能提供类似全功能`IDE(如 Visual Studio 或 CLion)`的体验：

* 代码补全 (Code Completion): 非常智能的提示，因为它真正理解代码的上下文（不仅仅是文本匹配）。
* 实时诊断 (Diagnostics): 在你输入代码时，实时显示编译错误和警告（速度极快）。
* 代码跳转 (Navigation): “转到定义” (Go to Definition)、“查找引用” (Find References)。
* 重构 (Refactoring): 比如重命名变量（Rename Symbol），它会智能地把作用域内所有相关的变量名都改掉，而不会误伤其他同名变量。
* 格式化 (Formatting): 集成了 clang-format，按行业标准格式化代码。
* 静态分析: 集成了 clang-tidy，可以检查潜在的 Bug 和代码风格问题。

## compile_commands.json

`clangd` 依赖 `compile_commands.json` 文件来了解项目的编译选项和文件结构。这个文件通常由构建系统生成，比如 `CMake`。

```CMake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

通过给 `CMake` 添加上述指令，可以在构建目录中生成 `compile_commands.json` 文件.

使用命令行参数`--compile-commands-dir=<路径>`就可以指定`compile_commands.json`文件的位置。

## VSCode配置Clangd

在`VSCode`中安装`Clangd`扩展后，可以通过以下步骤配置：

### 安装VSCode插件

1. 打开`VSCode`，进入扩展市场（Extensions Marketplace）。
2. 搜索`Clangd`。
3. 安装`llvm-vs-code-extensions.vscode-clangd`插件。

### 禁用内置C/C++扩展

1. 打开`VSCode`的扩展视图。
2. 搜索`C/C++`扩展（由Microsoft提供）。
3. 点击扩展旁的齿轮图标，选择`禁用`（Disable）。

### 配置Clangd路径

1. 打开`VSCode`设置（File -> Preferences -> Settings）。
2. 搜索`clangd`。
3. 在`Clangd: Path`字段中，输入`clangd`的安装路径（如果已添加到系统路径，可以直接输入`clangd`）。

### 配置编译选项

1. 在项目根目录下创建或编辑`.vscode/settings.json`文件。
2. 添加以下配置，指定`compile_commands.json`的位置：

    ```json
    {
        "clangd.arguments": [
            "--compile-commands-dir=${workspaceFolder}/build" // 替换为你的编译目录
        ]
    }
    ```

### 下载clangd

1. 打开[Clangd官方网站](https://clangd.llvm.org/installation.html)。
2. 下载适合你操作系统的预编译二进制文件。
3. 解压，将`bin`目录拷贝至`/usr/local/bin`或其他系统路径中,将`lib`目录拷贝至`/usr/local/lib`中。
4. 给予可执行权限`chmod +x /usr/local/bin/clangd`。
5. 在终端中运行`clangd --version`，确认安装成功。
