# C/C++程序编译流程

程序需要经过预处理、编译、汇编和链接这几个步骤才能变成可执行文件。

## 编译为`.o`文件

`C/C++`源文件需要首先编译为对应的`.o`文件，此时不需要解决所有的符号引用，只需要知道头文件里的声明就行了。

## 编译为库文件

当编译完毕`.o`文件后，如果是库文件，我们需要把它编译为库文件。库文件分为动态库和静态库。

在`Linux`中，动态库的后缀为`.so`,静态库的后缀为`.a`.

## 链接为可执行文件

把`.o`文件和所有的库文件一起编译成可执行文件，此时所有的符号引用必须解决。

使用选项`-llibrary`搜索库文件，也可以直接指定库文件作为参数。

## 静态链接

静态链接是把所有需要的库文件都编译进可执行文件中，生成的可执行文件体积较大，但运行时不依赖外部库文件。

本质上，静态库就是把多个`.o`文件打包在一起的文件，通常后缀名为`.a`。

### 设置库文件静态链接

通过编译器`-static`选项，可以将所有库文件静态链接到可执行文件中。例如：

```bash
gcc -static main.o -o main -lmylib
```

通过`static-libgcc`,`-static-libstdc++`选项可以将GCC和标准C++库也静态链接到可执行文件中。例如：

```bash
g++ -static-libgcc -static-libstdc++ main.o -o main -lmylib
```

## 动态链接

动态链接是把库文件在程序运行时加载进内存，生成的可执行文件体积较小，但运行时需要依赖外部库文件。

动态库文件通常后缀名为`.so`。

### 编译器自动进行动态链接

以编译`main.cpp`为例，当链接器发现程序调用了`printf`函数时,发现`printf`函数在`libc.so`动态库中有定义。此时

* 链接器不会将`printf`函数的代码复制到可执行文件中。
* 链接器会在可执行文件中记录`printf`函数所在的动态库`libc.so`的信息，比如`NEEDED`标签表示需要加载`libc.so`动态库、`PLT/GOT`表记录了`printf`函数在`libc.so`中的地址。

同理，链接器如果发现程序调用了其他动态库中的函数，默认也会进行动态链接。

### 显式运行时链接/动态加载

要显式运行时链接，库里的函数名必须是干净的。所以如果是`C++`代码，必须用`extern "C"`包裹，防止名字被修饰(Name Mangling)。

```CPP
#include <iostream>

// --- 这里是库的代码 ---
// libmy_plugin.so
// 使用 extern "C" 是为了让符号名保持为 "hello"，而不是 "_Z5helloPKc" 这种乱码
extern "C" {
    
    // 我们要导出的函数
    void hello(const char* name) {
        std::cout << "[Plugin] Hello, " << name << "! (From dynamic library)" << std::endl;
    }

    // 一个计算函数
    int add(int a, int b) {
        return a + b;
    }
}
```

加载器使用`Linux`标准的`dlopen`, `dlsym`, `dlclose`函数来加载动态库并获取函数地址。

```CPP
#include <iostream>
#include <dlfcn.h> // 核心头文件：提供 dlopen, dlsym 等

// 1. 定义函数指针类型
//    这必须与库里函数的签名完全一致，否则栈内存会错乱
typedef void (*HelloFunc)(const char*);
typedef int  (*AddFunc)(int, int);

int main() {
    const char* lib_path = "./libmy_plugin.so";

    // ---------------------------------------------------------
    // 步骤 1: 打开动态库 (dlopen)
    // ---------------------------------------------------------
    // RTLD_LAZY: 延迟解析。只有当你真正去 resolve 符号时才工作。
    // RTLD_NOW:  立即解析。加载时如果有符号找不到，立刻报错。
    void* handle = dlopen(lib_path, RTLD_LAZY);

    if (!handle) {
        std::cerr << "无法加载库: " << dlerror() << std::endl;
        return 1;
    }
    std::cout << ">>> 库加载成功！句柄: " << handle << std::endl;

    // ---------------------------------------------------------
    // 步骤 2: 清除之前的错误 (可选但推荐)
    // ---------------------------------------------------------
    dlerror(); 

    // ---------------------------------------------------------
    // 步骤 3: 查找函数符号 (dlsym)
    // ---------------------------------------------------------
    // 我们找名为 "hello" 的函数
    // dlsym 返回 void*，必须强制转换为对应的函数指针类型
    HelloFunc hello_func = (HelloFunc)dlsym(handle, "hello");

    // 检查是否找到
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "找不到符号 'hello': " << dlsym_error << std::endl;
        dlclose(handle);
        return 1;
    }

    // 同样的方法找 "add" 函数
    AddFunc add_func = (AddFunc)dlsym(handle, "add");
    if (!add_func) { // 也可以直接判空来检查
        std::cerr << "找不到符号 'add'" << std::endl;
        dlclose(handle);
        return 1;
    }

    // ---------------------------------------------------------
    // 步骤 4: 调用函数
    // ---------------------------------------------------------
    std::cout << ">>> 准备调用函数..." << std::endl;
    
    hello_func("Robot Engineer"); // 调用插件里的函数
    
    int result = add_func(10, 20);
    std::cout << ">>> 计算结果: " << result << std::endl;

    // ---------------------------------------------------------
    // 步骤 5: 关闭库 (dlclose)
    // ---------------------------------------------------------
    // 关闭后，handle 失效，之前获取的函数指针也失效，不能再调用
    dlclose(handle);
    std::cout << ">>> 库已卸载。" << std::endl;

    return 0;
}
```

#### 函数说明

`dlopen`

```CPP
void* dlopen(const char* filename, int flag);
```

用于加载动态库，返回一个句柄。如果加载失败，返回`NULL`。

* `filename`：动态库的路径,如果包含`/`则直接按路径查找，否则按`LD_LIBRARY_PATH`环境变量和默认路径查找。
* `flag`：加载选项，常用的有`RTLD_LAZY`(延迟解析)和`RTLD_NOW`(立即解析)。

`dlsym`

```CPP
void* dlsym(void* handle, const char* symbol);
```

用于查找动态库中的符号(函数或变量)，返回符号的地址。如果找不到，返回`NULL`。

* `handle`：由`dlopen`返回的动态库句柄。
* `symbol`：要查找的符号名。

`dlclose`

```CPP
int dlclose(void* handle);
```

用于关闭动态库，释放资源。成功返回`0`，失败返回非`0`。

#### RAII封装

手动管理动态库的打开和关闭比较麻烦，也无法保证异常安全。我们可以用RAII封装一个类来自动管理动态库的生命周期。

```CPP
class SharedLibrary {
public:
    SharedLibrary(const std::string& path) {
        handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) throw std::runtime_error(dlerror());
    }

    ~SharedLibrary() {
        if (handle) dlclose(handle); // 自动关闭，防泄漏
    }

    template <typename T>
    T get_function(const std::string& name) {
        void* sym = dlsym(handle, name.c_str());
        if (!sym) throw std::runtime_error(dlerror());
        return reinterpret_cast<T>(sym);
    }

private:
    void* handle;
};

// 使用：
// SharedLibrary lib("./libmy_plugin.so");
// auto func = lib.get_function<AddFunc>("add");
```

#### 宏封装

为了简化`dlopen`和`dlsym`的使用，可以定义一些宏来封装常用操作：

```CPP
#define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
#define LOAD_SYMBOL(handle, symbol) dlsym(handle, symbol)
#define CLOSE_LIBRARY(handle) dlclose(handle)
```

在许多项目中，这样的宏定义可以提高代码的可读性和可维护性。

比如`pulseaudio`项目中就使用了类似的宏封装动态库加载。

```CPP
#define PA_MODULE_LOAD(name, path, ...) \
    do { \
        void* handle = LOAD_LIBRARY(path); \
        if (!handle) { \
            fprintf(stderr, "无法加载模块 %s: %s\n", name, dlerror()); \
            break; \
        } \
        /* 其他初始化代码 */ \
    } while(0)
```

比如`pluginlib`项目中也使用了类似的宏封装动态库加载。

```CPP
// 你的代码中写了这一行
PLUGINLIB_EXPORT_CLASS(MyRobotController, nav_core::BaseLocalPlanner)
```

这行宏经过 预处理器 (Preprocessor) 展开后，实际上变成了类似下面这样的 C++ + C 混合代码（简化逻辑版）：

```CPP
// --- 宏展开后的真实面目 ---

// 1. 这是一个普通的 C++ 类构造逻辑
class MyRobotController : public nav_core::BaseLocalPlanner { ... };

// 2. 【核心】这里生成了一个 dlsym 能找到的 C 函数！
extern "C" {
    // 这个函数名通常是框架约定好的，或者由宏根据类名拼接出来的
    // 它的作用只有一个：充当“工厂”，New 一个对象出来
    nav_core::BaseLocalPlanner* create_instance_of_my_robot_controller() {
        return new MyRobotController();
    }
    
    // 同时也得有一个销毁函数
    void destroy_instance(nav_core::BaseLocalPlanner* p) {
        delete p;
    }
}
```

### 编译动态库

编译动态库时，需要使用`-shared`选项，并指定生成的动态库文件名。例如：

```bash
g++ -shared -o libmy_plugin.so -fPIC my_plugin.cpp
```

这里的`-fPIC`选项用于生成与位置无关的代码(Position Independent Code)，这是创建动态库时的常见要求。

### PLT/GOT表

在动态链接中，PLT（Procedure Linkage Table）和GOT（Global Offset Table）是两个关键的数据结构，用于实现函数调用的动态解析。

* **PLT（过程链接表）**：PLT是一个跳转表，存储了函数调用的入口地址。当程序调用一个动态库中的函数时，实际上是先跳转到PLT中的对应入口，然后通过GOT来获取实际的函数地址。如果函数地址还没有解析，PLT会调用动态链接器来解析地址，并更新GOT。
* **GOT（全局偏移表）**：GOT是一个存储全局变量和函数地址的表格。每个动态库中的函数和全局变量在GOT中都有一个条目，存储它们的实际地址。初始时，这些地址可能是未定义的，只有在第一次调用时才会被解析并填充。

PLT/GOT表都是在用户空间中维护的，每个进程每个动态库都有自己的PLT/GOT表。但是PLT表通常指向同一个动态库的函数地址。

任何ELF文件，只要它想调用自己以外的代码，它就需要PLT/GOT表。

#### 内存布局示意图

##### 可执行文件

可执行文件`main`,内存布局如下：

* `.text`包含代码段，包括PLT表。
* `.got`包含GOT表。

##### 动态库

动态库`libc.so`,内存布局如下：

* `.text`包含代码段。
* `.plt`包含PLT表。
* `.got`包含GOT表。

#### 实例

当程序执行到`call printf@plt`时，实际执行的步骤如下：

1. 跳转到PLT中的`printf`入口。
2. PLT检查GOT中`printf`的地址是否已经解析。
3. 如果未解析，调用动态链接器来查找`printf`的实际地址。
4. 动态链接器找到`printf`的地址后，更新GOT中的条目。
5. 最后，跳转到`printf`的实际地址执行函数。

### ABI兼容性

在使用动态库时，必须确保编译器和库的ABI（应用二进制接口）兼容。不同版本的编译器可能会生成不同的ABI，导致符号解析失败或运行时错误。

通常在Linux的编译器中，都遵循`Itanium C++ ABI`标准，但不同版本的GCC或Clang可能会有细微差别。

#### 脆弱基类问题

假设`SensorInterface.h`定义了一个基类：

```CPP
// SensorInterface.h
#pragma once
#include <iostream>

class SensorInterface {
public:
    // 【关键】必须有虚析构函数，否则 delete 基类指针时不会调用子类析构
    virtual ~SensorInterface() {}

    virtual void init() = 0;
    virtual void read_data() = 0;
};

// 定义工厂函数的函数指针类型，方便后续转换
typedef SensorInterface* (*CreateFunc)();
typedef void (*DestroyFunc)(SensorInterface*);
```

用户按照这个接口写了很多插件。此时，如果后来我们给`SensorInterface`类添加了一个新的虚函数：

```CPP
class SensorInterface {
public:
    virtual ~SensorInterface() {}
    virtual void init() = 0;
    virtual void calibrate() = 0; // <--- 新增的！
    virtual void read_data() = 0;
};
```

1. 主程序重新编译了： 它认为 read_data 现在是 第 3 项。
2. 用户的插件没重新编译： 它的 vtable 里，read_data 依然在 第 2 项。

当主程序调用`sensor->read_data()`（查第 3 项）时，它可能会读到内存里的垃圾数据，或者跳到了错误的代码地址。

### Pimpl (Pointer to Implementation) 模式

使用`Pimpl`模式可以隐藏类的实现细节，减少ABI变化对用户代码的影响。

将代码分为三个部分

1. `Robot.h`：类的声明，包含一个指向实现类的指针。
2. `Robot.cpp`：类的实现，定义实现类的具体内容。
3. `main.cpp`：使用类的代码。

#### Robot.h

```CPP
#pragma once
#include <memory>

class Robot {
public:
    Robot();
    ~Robot(); // 【关键】析构函数必须在 .cpp 里实现

    // 公开的接口方法
    void move(double x, double y);
    void stop();
    double get_battery_life() const;

    // 禁止拷贝 (因为 unique_ptr 不能拷贝，除非你手动实现深拷贝)
    Robot(const Robot&) = delete;
    Robot& operator=(const Robot&) = delete;

private:
    // 前向声明 (Forward Declaration)
    class RobotImpl; 
    
    // 指向实现的指针
    // 使用 unique_ptr 自动管理内存，防止内存泄漏
    std::unique_ptr<RobotImpl> pImpl;
};
```

析构函数必须在`.cpp`文件中定义，因为此时`RobotImpl`才是完整类型，`unique_ptr`才能正确调用它的析构函数。

#### Robot.cpp

```CPP
#include "Robot.h"
#include <iostream>
#include <string>

// 这个类的具体结构只有 Robot.cpp 知道，外界完全不可见
class Robot::RobotImpl {
public:
    // 这里可以随意添加、删除成员变量，完全不影响 Robot.h 的二进制兼容性
    double current_x = 0.0;
    double current_y = 0.0;
    double battery_level = 100.0;
    std::string hardware_id = "REV-1"; // 甚至可以用 std::string 这种复杂对象

    void internal_hardware_check() {
        std::cout << "Checking hardware: " << hardware_id << "...\n";
    }
};

// --- 下面是 Robot 类的实现 ---

// 构造函数：初始化 pImpl
Robot::Robot() : pImpl(std::make_unique<RobotImpl>()) {
    std::cout << "[Robot] Constructed.\n";
}

// 【关键点】析构函数
// 必须在这里定义。因为此时 RobotImpl 才是完整类型，unique_ptr 才能正确调用它的析构。
Robot::~Robot() = default;

void Robot::move(double x, double y) {
    // 通过指针调用具体的实现
    pImpl->current_x += x;
    pImpl->current_y += y;
    pImpl->battery_level -= 1.5;
    
    std::cout << "Moving to (" << pImpl->current_x << ", " << pImpl->current_y << ")\n";
    pImpl->internal_hardware_check();
}

void Robot::stop() {
    std::cout << "Robot Stopped.\n";
}

double Robot::get_battery_life() const {
    return pImpl->battery_level;
}
```

#### main.cpp

```CPP
#include "Robot.h"
#include <iostream>

int main() {
    Robot my_robot;
    
    my_robot.move(10.5, 20.0);
    std::cout << "Battery: " << my_robot.get_battery_life() << "%\n";
    
    return 0;
}
```

#### 总结

通过使用`Pimpl`模式，我们实现了以下目标：

1. **隐藏实现细节**：用户无法看到`RobotImpl`的定义，减少了对实现细节的依赖。
2. **减少ABI变化影响**：添加或修改`RobotImpl`的成员变量不会影响`Robot`类的二进制接口，用户代码无需重新编译。
3. **简化头文件**：头文件中只包含必要的声明，减少了编译依赖，加快编译速度。

#### 性能开销

使用`Pimpl`模式会引入一些性能开销，主要体现在以下几个方面：

1. **间接访问开销**：每次访问实现类的成员变量或方法时，都需要通过指针进行间接访问，这比直接访问成员变量稍慢。
2. **内存分配开销**：`Pimpl`模式通常使用动态内存分配（如`new`），这会带来一定的内存分配和释放开销。
3. **缓存局部性降低**：由于实现类的成员变量分散在堆内存中，可能会导致缓存局部性降低，影响性能。

尽管存在这些开销，但在大多数应用场景中，这些性能影响是可以接受的，尤其是当需要频繁修改类的实现时，`Pimpl`模式带来的灵活性和二进制兼容性优势往往超过了性能损失。

#### const传递性

`const`传递性是指当一个对象被声明为`const`时，其成员函数也应该被声明为`const`，以保证在调用这些成员函数时不会修改对象的状态。

```CPP
// Robot.cpp
double Robot::get_battery_life() const {
    // 这里的 this 是 const Robot*
    // 所以 pImpl 变成了 const std::unique_ptr<RobotImpl>
    
    // 【关键点】：
    // const unique_ptr<T> 等价于 T* const （指针本身是常量，不可改变指向）
    // 它并不等价于 const T* （指针指向的内容是常量）
    
    pImpl->battery_level = 0.0; // 竟然编译通过了！而且修改了数据！
    
    return pImpl->battery_level;
}
```

解决方法是

```CPP
class Robot {
    // ...
private:
    class RobotImpl;
    std::unique_ptr<RobotImpl> pImpl;

    // 【核心技巧】定义两个私有访问器
    RobotImpl* impl() { return pImpl.get(); }
    const RobotImpl* impl() const { return pImpl.get(); }
};
```

定义两个私有访问器函数`impl()`，一个用于非`const`对象，另一个用于`const`对象。这样，在`const`成员函数中调用`impl()`时，会返回一个指向`const RobotImpl`的指针，从而防止修改实现类的成员变量。

或者是C++17引入的

```CPP
#include <memory>
#include <experimental/propagate_const> // 需要包含这个头文件

class Robot {
    // ...
private:
    class RobotImpl;
    // 用 propagate_const 包裹你的智能指针
    std::experimental::propagate_const<std::unique_ptr<RobotImpl>> pImpl;
};
```

使用`std::experimental::propagate_const`可以自动处理`const`传递性问题。当`Robot`对象是`const`时，`pImpl`会被视为指向`const RobotImpl`的指针，防止修改实现类的成员变量。
