# Pulseaudio笔记

Pulseaudio是一个跨平台的声音服务器，常用于Linux系统中。它提供了高级的音频功能，如音频混合、网络音频传输和音频设备管。

## 概念

### 模块

模块是Pulseaudio的核心组件，实际上是动态加载的共享库。每个模块实现了特定的功能，如音频输入、输出、混音等。常见的模块包括：

* `module-alsa-sink`：用于将音频输出到ALSA设备。
* `module-alsa-source`：用于从ALSA设备获取音频输入。
* `module-loopback`：用于将音频从一个源路由到另一个接收器。
* `module-null-sink`：创建一个虚拟的音频接收器，常用于测试。
* `module-combine-sink`：将多个音频输出设备合并为一个虚拟设备。
* `module-remap-sink`：用于重新映射音频通道。
* `module-native-protocol-tcp`：允许通过TCP协议远程连接到Pulseaudio服务器。
* `module-zeroconf-publish`：用于通过Zeroconf发布Pulseaudio服务，方便网络发现。

### Sink

`Sink`是Pulseaudio中的音频输出设备。它接收来自源（Source）的音频数据并将其发送到物理设备（如扬声器或耳机）。

### Source

`Source`是Pulseaudio中的音频输入设备。它捕获来自物理设备（如麦克风）的音频数据并将其发送到接收器（Sink）或应用程序。

### Sink Input

`Sink Input`是指应用程序或音频流向`Sink`发送音频数据的连接。每个`Sink Input`代表一个独立的音频流，可以单独控制其音量和其他属性。

### Source Output

`Source Output`是指从`Source`捕获音频数据并发送到应用程序或其他处理单元的连接。每个`Source Output`代表一个独立的音频流，可以单独控制其属性。

### Card

`Card`表示物理音频设备，如声卡。它包含多个`Sink`和`Source`，并提供对这些设备的管理和配置功能。

### Profile

`Profile`定义了`Card`的工作模式，指定了哪些`Sink`和`Source`可用以及它们的配置。例如，一个声卡可能有多个配置文件，如立体声输出、环绕声输出等。

### Virtual Device

`Virtual Device`是通过软件创建的虚拟音频设备，允许用户在不依赖物理硬件的情况下进行音频处理和路由。例如，`module-null-sink`创建了一个虚拟的音频接收器，可以用于测试或将音频流重定向到其他处理单元。

### Hooks

`Hooks`是Pulseaudio中的回调机制，允许模块在特定事件发生时执行自定义代码。例如，当音频流开始或停止时，模块可以通过注册钩子函数来响应这些事件，从而实现动态的音频处理和管理。

### Proplist

`Proplist`是Pulseaudio中用于存储属性的键值对集合。它用于描述音频设备、流和其他对象的元数据，如名称、描述、格式等。`Proplist`允许模块和应用程序以结构化的方式访问和修改这些属性，从而实现更灵活的音频管理和配置。

### Mainloop

`Pulseaudio`是单线程事件循环架构，这意味着模块代码(初始化，回调函数)和`Pulseaudio`处理其他命令的代码是在同一个线程里排队执行。如果要做复杂的操作需要新开一个线程或者使用`Mainloop API`来避免阻塞`Pulseaudio`的主循环。

### 模块开发流程

* 加载(init)模块，初始化相关数据结构，注册回调函数等。
* 监听(Callbacks)事件，如果回调函数被触发，执行相应的处理逻辑。
* 卸载(done)模块，释放资源，注销回调函数等。

## Hooks

`pulseaudio`的`hooks`定义在`<pulsecore/core.h>`中，常用的`hooks`包括：

### 监听流的生命周期(stream lifecycle)

* `PA_CORE_HOOK_SINK_INPUT_PUT`
  * 触发时机：当一个新的`Sink Input`被创建并添加到`Sink`时触发。比如APP开始播放。
  * call_data类型：`pa_sink_input*`,指向新创建的`Sink Input`对象。
  * 用途：用于在新的音频流被添加时执行自定义操作，如初始化相关资源或记录日志。
* `PA_CORE_HOOK_SINK_INPUT_UNLINK`
  * 触发时机：当一个`Sink Input`被移除或断开连接时触发。比如APP停止播放。
  * call_data类型：`pa_sink_input*`,指向被移除的`Sink Input`对象。
  * 用途：用于在音频流被移除时执行清理操作，如释放资源或更新状态。
* `PA_CORE_HOOK_SOURCE_OUTPUT_PUT`
  * 触发时机：当一个新的`Source Output`被创建并添加到`Source`时触发。比如APP开始录音。
  * call_data类型：`pa_source_output*`,指向新创建的`Source Output`对象。
  * 用途：用于在新的音频输入流被添加时执行自定义操作，如初始化相关资源或记录日志。
* `PA_CORE_HOOK_SOURCE_OUTPUT_UNLINK`
  * 触发时机：当一个`Source Output`被移除或断开连接时触发。比如APP停止录音。
  * call_data类型：`pa_source_output*`,指向被移除的`Source Output`对象。
  * 用途：用于在音频输入流被移除时执行清理操作，如释放资源或更新状态。

### 监听设备的变化(device changes)

* `PA_CORE_HOOK_SINK_PUT`
  * 触发时机：当一个新的`Sink`被创建并添加到系统时触发。比如插入了USB声卡。
  * call_data类型：`pa_sink*`,指向新创建的`Sink`对象。
  * 用途：用于在新的音频输出设备被添加时执行自定义操作，如初始化相关资源或记录日志。
* `PA_CORE_HOOK_SINK_UNLINK`
  * 触发时机：当一个`Sink`被移除或断开连接时触发。比如拔出了USB声卡。
  * call_data类型：`pa_sink*`,指向被移除的`Sink`对象。
  * 用途：用于在音频输出设备被移除时执行清理操作，如释放资源或更新状态
* `PA_CORE_HOOK_SOURCE_PUT`
  * 触发时机：当一个新的`Source`被创建并添加到系统时触发。比如插入了麦克风。
  * call_data类型：`pa_source*`,指向新创建的`Source`对象。
  * 用途：用于在新的音频输入设备被添加时执行自定义操作，如初始化相关资源或记录日志。
* `PA_CORE_HOOK_SOURCE_UNLINK`
  * 触发时机：当一个`Source`被移除或断开连接时触发。比如拔出了麦克风。
  * call_data类型：`pa_source*`,指向被移除的`Source`对象。
  * 用途：用于在音频输入设备被移除时执行清理操作，如释放资源或更新状态。

### 监听状态的变化(state changes)

* `PA_CORE_HOOK_SINK_INPUT_STATE_CHANGED`
  * 触发时机：当一个`Sink Input`的状态发生变化时触发。例如，从暂停状态变为播放状态。
  * call_data类型：`pa_sink_input*`,指向状态发生变化的`Sink Input`对象。
  * 用途：用于在音频流状态变化时执行自定义操作，如更新UI或调整资源分配。
* `PA_CORE_HOOK_SOURCE_OUTPUT_STATE_CHANGED`
  * 触发时机：当一个`Source Output`的状态发生变化时触发。例如，从暂停状态变为录音状态。
  * call_data类型：`pa_source_output*`,指向状态发生变化的`Source Output`对象。
  * 用途：用于在音频输入流状态变化时执行自定义操作，如更新UI或调整资源分配。
* `PA_CORE_HOOK_SINK_STATE_CHANGED`
  * 触发时机：当一个`Sink`的状态发生变化时触发。例如，从空闲状态变为活动状态。
  * call_data类型：`pa_sink*`,指向状态发生变化的`Sink`对象。
  * 用途：用于在音频输出设备状态变化时执行自定义操作，如调整音量或更新设备列表。
* `PA_CORE_HOOK_SOURCE_STATE_CHANGED`
  * 触发时机：当一个`Source`的状态发生变化时触发。例如，从空闲状态变为活动状态。
  * call_data类型：`pa_source*`,指向状态发生变化的`Source`对象。
  * 用途：用于在音频输入设备状态变化时执行自定义操作，如调整增益或更新设备列表。
* `PA_CORE_HOOK_SINK_INPUT_VOLUME_CHANGED`
  * 触发时机：当一个`Sink Input`的音量发生变化时触发。
  * call_data类型：`pa_sink_input*`,指向音量发生变化的`Sink Input`对象。
  * 用途：用于在音频流音量变化时执行自定义操作，如更新UI或调整混音设置。
* `PA_CORE_HOOK_SINK_INPUT_MUTE_CHANGED`
  * 触发时机：当一个`Sink Input`的静音状态发生变化时触发。
  * call_data类型：`pa_sink_input*`,指向静音状态发生变化的`Sink Input`对象。
  * 用途：用于在音频流静音状态变化时执行自定义操作，如更新UI或调整混音设置。
* `PA_CORE_HOOK_SOURCE_OUTPUT_VOLUME_CHANGED`
  * 触发时机：当一个`Source Output`的音量发生变化时触发。
  * call_data类型：`pa_source_output*`,指向音量发生变化的`Source Output`对象。
  * 用途：用于在音频输入流音量变化时执行自定义操作，如更新UI或调整录音设置。
* `PA_CORE_HOOK_SOURCE_OUTPUT_MUTE_CHANGED`
  * 触发时机：当一个`Source Output`的静音状态发生变化时触发。
  * call_data类型：`pa_source_output*`,指向静音状态发生变化的`Source Output`对象。
  * 用途：用于在音频输入流静音状态变化时执行自定义操作，如更新UI或调整录音设置。

### 监听路由的变化(routing changes)

* `PA_CORE_HOOK_SINK_INPUT_MOVE_START`
  * 触发时机：当一个`Sink Input`开始从一个`Sink`移动到另一个`Sink`时触发。
  * call_data类型：`pa_sink_input*`,指向正在移动的`Sink Input`对象。
  * 用途：用于在音频流开始路由变化时执行自定义操作，如记录日志或准备资源。
* `PA_CORE_HOOK_SINK_INPUT_MOVE_FINISH`
  * 触发时机：当一个`Sink Input`完成从一个`Sink`移动到另一个`Sink`时触发。
  * call_data类型：`pa_sink_input*`,指向已完成移动的`Sink Input`对象。
  * 用途：用于在音频流完成路由变化时执行自定义操作，如更新状态或释放资源。
* `PA_CORE_HOOK_SOURCE_OUTPUT_MOVE_START`
  * 触发时机：当一个`Source Output`开始从一个`Source`移动到另一个`Source`时触发。
  * call_data类型：`pa_source_output*`,指向正在移动的`Source Output`对象。
  * 用途：用于在音频输入流开始路由变化时执行自定义操作，如记录日志或准备资源。
* `PA_CORE_HOOK_SOURCE_OUTPUT_MOVE_FINISH`
  * 触发时机：当一个`Source Output`完成从一个`Source`移动到另一个`Source`时触发。
  * call_data类型：`pa_source_output*`,指向已完成移动的`Source Output`对象。
  * 用途：用于在音频输入流完成路由变化时执行自定义操作，如更新状态或释放资源。

### 监听模块的变化

* `PA_CORE_HOOK_MODULE_NEW`
  * 触发时机：`pa_module_load()`函数刚开始执行，模块对象已创建，但可能还未完全初始化完成。
  * call_data类型：`pa_module*`,指向新加载的模块对象。
  * 用途：用于在模块加载时执行自定义操作，如初始化相关资源或记录日志。
* `PA_CORE_HOOK_MODULE_UNLINK`
  * 触发时机：`pa_module_unload()`函数刚开始执行，模块对象仍然存在，但即将被卸载。
  * call_data类型：`pa_module*`,指向即将卸载的模块对象。
  * 用途：用于在模块卸载时执行清理操作，如释放资源或更新状态。

### 连接hooks

```cpp
pa_hook_slot* pa_hook_connect(
    pa_hook *hook,              // 1. 目标钩子链表 (你要挂哪里？)
    pa_hook_priority_t prio,    // 2. 优先级 (你要排第几？)
    pa_hook_cb_t cb,            // 3. 回调函数 (你的函数指针)
    void *data                  // 4. 用户数据 (传给回调的 userdata)
);
```

在`module-init`函数中使用`pa_core_hook_connect()`连接钩子。

参数

* `hook`：指定要连接的钩子链表，可以通过`pa_core_get_hook()`函数获取。
* `prio`：指定钩子的优先级，决定了钩子函数的执行顺序。常用的优先级有`PA_HOOK_EARLY`、`PA_HOOK_NORMAL`和`PA_HOOK_LATE`。
* `cb`：指定钩子函数的回调函数，当钩子被触发时会调用该函数。
* `data`：指定用户数据，该数据会传递给钩子函数的`userdata`参数。

返回值

* 返回一个指向`pa_hook_slot`的指针，表示已连接的钩子槽。如果连接失败，返回`NULL`。

### 断开hooks

```cpp
void pa_hook_slot_free(pa_hook_slot *slot);
```

在`module-done`函数中使用`pa_hook_slot_free()`断开钩子。

参数

* `slot`：指定要断开的钩子槽，可以在连接钩子时获得。

### 示例

定义`userdata`数据结构体用于保存`slot`指针

```C
struct userdata {
    pa_hook_slot *sink_input_put_slot; // 用来保存句柄
    // ... 其他数据 ...
};
```

在`pa__init`函数中连接钩子

```C
// 你的回调函数
static pa_hook_result_t on_sink_input_put(pa_core *c, pa_sink_input *si, void *u) {
    pa_log("有人开始播放了！");
    return PA_HOOK_OK;
}

int pa__init(pa_module *m) {
    struct userdata *u = pa_xnew0(struct userdata, 1);
    m->userdata = u;

    // 【核心代码】 连接钩子
    u->sink_input_put_slot = pa_hook_connect(
        &m->core->hooks[PA_CORE_HOOK_SINK_INPUT_PUT], // 1. 目标：从 core 的数组里找
        PA_HOOK_NORMAL,                               // 2. 优先级：普通
        on_sink_input_put,                            // 3. 动作：你的函数
        u                                             // 4. 参数：传给回调
    );

    // 检查是否成功 (通常只要内存够都会成功)
    if (!u->sink_input_put_slot) {
        pa_log("钩子挂载失败！");
        return -1;
    }

    return 0;
}
```

在`pa__done`函数中断开钩子

```C
void pa__done(pa_module *m) {
    struct userdata *u = m->userdata;

    if (!u) return;

    // 【核心代码】 释放钩子
    if (u->sink_input_put_slot) {
        pa_hook_slot_free(u->sink_input_put_slot);
        u->sink_input_put_slot = NULL; // 好习惯：置空防止重复释放
    }

    pa_xfree(u);
}
```

## 参数系统

`Pulseaudio`的参数系统允许模块在加载时接收配置参数。这些参数可以通过模块加载命令行传递，也可以通过配置文件指定

相关函数定义在`<pulsecore/modargs.h>`中。

读取参数流程为：

1. 定义参数白名单：列出所有可以识别的参数名
2. 解析字符串：将`m->argument`字符串转化为`pa_modargs`对象
3. 读取数值：从`pa_modargs`对象中按照类型提取数据
4. 释放内存：销毁`pa_modargs`对象

### 定义识别的参数

通过一个以`NULL`结尾的字符串数组定义参数白名单。例如：

```C
static const char* const valid_modargs[] = {
    "device",       // 字符串类型
    "rate",         // 整数类型
    "enable_ai",    // 布尔类型
    NULL
};
```

### 解析参数字符串

在`pa__init`函数中使用`pa_modargs_new()`解析参数字符串：

```C
pa_modargs *pa_modargs_new(const char *argument, const char* const valid_modargs[]);
```

示例为：

```C
pa_modargs *ma = pa_modargs_new(m->argument, valid_modargs);
if (!ma) {
    pa_log("无效的模块参数！");
    return -1;
}
```

### 按照类型读取参数值

使用`pa_modargs_get_*`函数从`pa_modargs`对象中提取参数值：

```C
// A. 读取字符串 (String)
// 直接返回指针，如果没有该参数则返回 NULL
const char *dev_name = pa_modargs_get_value(ma, "device", "default"); // 第3个参数是默认值
pa_log_info("设备名: %s", dev_name);

// B. 读取整数 (Unsigned Integer)
uint32_t rate = 44100; // 默认值
// 注意：返回值是错误码，而不是结果！结果通过指针传出。
if (pa_modargs_get_value_u32(ma, "rate", &rate) < 0) {
    pa_log_error("rate 参数格式错误 (不是数字)");
    goto fail;
}
pa_log_info("采样率: %u", rate);

// C. 读取布尔值 (Boolean)
// 支持 "1", "true", "yes", "on" 为真
// 支持 "0", "false", "no", "off" 为假
bool ai_enabled = false;
if (pa_modargs_get_value_boolean(ma, "enable_ai", &ai_enabled) < 0) {
    pa_log_error("enable_ai 参数格式错误");
    goto fail;
}
pa_log_info("AI 功能: %s", ai_enabled ? "开启" : "关闭");
```

### 释放参数对象

使用`pa_modargs_free()`释放`pa_modargs`对象：

```C
void pa_modargs_free(pa_modargs *ma);
```

示例为：

```C
pa_modargs_free(ma);
```

## 异步消息队列pa_asyncmsgq

Pulseaudio使用异步消息队列(`pa_asyncmsgq`)在不同模块和组件之间传递消息。这种机制允许模块在不阻塞主循环的情况下进行通信和协调。

`pa_asyncmsgq`定义在`<pulsecore/asyncmsgq.h>`中。

### 核心特性

* 多生产者单消费者(MPSC)模型：允许多个线程安全地向队列发送消息，而只有一个线程负责接收和处理消息。
* 支持异步也支持同步消息：可以选择发送异步消息（不等待处理结果）或同步消息（等待处理结果）。
* 基于对象的路由：队列里的每一个消息都包含`pa_msgobject`指针，指明消息的接收者对象，消息代码`OpCode`，以及可选的参数和回调函数。
* 使用引用计数管理内存：消息和对象使用引用计数来确保在使用过程中不会被意外释放。

### 创建消息队列

```C
pa_asyncmsgq* pa_asyncmsgq_new(unsigned size);
```

创建一个新的异步消息队列。

* `size`：指定队列的大小（消息数量）。`0`表示默认值

返回值

* 返回一个指向新创建的`pa_asyncmsgq`对象的指针。如果创建失败，返回`NULL`。

### 销毁消息队列

```C
void pa_asyncmsgq_unref(pa_asyncmsgq *q);
```

将消息队列的引用计数减1，如果引用计数为0，则销毁队列。

### 异步投递

```C
void pa_asyncmsgq_post(
    pa_asyncmsgq *q,           // 目标队列
    pa_msgobject *object,      // 接收该消息的对象 (比如 sink, source)
    int code,                  // 消息 ID (你自己定义的枚举)
    const void *data,          // 数据指针 (可选)
    int64_t offset,            // 辅助数据 (可选)
    const pa_memchunk *chunk,  // 内存块 (可选，用于传递音频数据片段)
    pa_free_cb_t free_cb       // 释放回调 (关键！)
);
```

将一条异步消息投递到消息队列。

参数

* `q`：目标消息队列。
* `object`：接收该消息的对象，通常是一个`pa_msgobject`的子类实例，如`sink`或`source`。
* `code`：消息 ID，用于标识消息的类型。可以是自定义的枚举值。
* `data`：指向消息数据的指针，可以传递任何类型的数据。如果不需要传递数据，可以传递`NULL`。
* `offset`：辅助数据，可以用于传递额外的信息，如时间戳等。如果不需要，可以传递`0`。
* `chunk`：指向一个`pa_memchunk`对象的指针，用于传递音频数据片段。如果不需要传递音频数据，可以传递`NULL`。
* `free_cb`：释放回调函数，当消息处理完成后调用该函数释放相关资源。

### 同步发送

```C
int pa_asyncmsgq_send(
    pa_asyncmsgq *q,
    pa_msgobject *object,
    int code,
    const void *data,
    int64_t offset,
    const pa_memchunk *chunk
);
```

将一条同步消息发送到消息队列，并等待处理结果。

参数

* `q`：目标消息队列。
* `object`：接收该消息的对象，通常是一个`pa_msgobject`的子类实例，如`sink`或`source`。
* `code`：消息 ID，用于标识消息的类型。可以是自定义的枚举值。
* `data`：指向消息数据的指针，可以传递任何类型的数据。如果不需要传递数据，可以传递`NULL`。
* `offset`：辅助数据，可以用于传递额外的信息，如时间戳等。如果不需要，可以传递`0`。
* `chunk`：指向一个`pa_memchunk`对象的指针，用于传递音频数据片段。如果不需要传递音频数据，可以传递`NULL`。

返回值

* 返回`0`表示消息发送和处理成功，返回负值表示发生错误。

### 接收

```C
int pa_asyncmsgq_read(pa_asyncmsgq *q);
```

查看消息队列中是否有可用的消息，并处理它们。

参数

* `q`：目标消息队列。

返回值

* 返回`0`表示成功处理了一条消息，返回负值表示发生错误，返回正值表示没有可用的消息。

通常配合`pa_rtpoll`使用.

### pa_msgobject结构体

`pa_msgobject`是Pulseaudio中用于消息传递的基础对象。它定义在`<pulsecore/msgobject.h>`中。

类似于`C++`中的继承，`sink`,`source`,`module`等结构体都包含了一个`pa_msgobject`作为它们的第一个成员，从而实现了多态行为。

## 内存块管理pa_memblock

`pa_memblock`是Pulseaudio中用于管理内存块的结构体。它定义在`<pulsecore/memblock.h>`中。

`pa_memblock`用于高效地分配和管理内存，特别是在处理音频数据时。它提供了引用计数机制，允许多个组件共享同一块内存，而无需复制数据，从而提高性能。

### 生命周期管理

```C
pa_memblock* pa_memblock_new(pa_mempool *p, size_t length);
```

从指定的内存池中分配一块新的内存块。

* `p`：指向内存池的指针。
* `length`：要分配的内存块的大小（以字节为单位）。

返回一个指向新分配的`pa_memblock`对象的指针。此时的`RefCount`为1。

```C
pa_memblock* pa_memblock_ref(pa_memblock *b);
```

将内存块的引用计数加1。

* `b`：指向要增加引用计数的内存块的指针。

返回同一个内存块指针。

```C
void pa_memblock_unref(pa_memblock *b);
```

将内存块的引用计数减1，如果引用计数为0，则释放内存块。

* `b`：指向要减少引用计数的内存块的指针。

### 访问内存块数据

`pa_memblock`是一个结构体，需要先获取数据指针后才能访问其内容。

```C
void* pa_memblock_acquire(pa_memblock *b);
```

获取内存块的可读写指针。

* `b`：指向要获取数据指针的内存块的指针。如果这是共享内存（SHM），这一步可能会触发`mmap`系统调用，把文件映射到进程空间。

返回一个指向内存块数据的指针。

```C
void pa_memblock_release(pa_memblock *b);
```

释放通过`pa_memblock_acquire()`获取的数据指针。

* `b`：指向要释放数据指针的内存块的指针。

### 状态查询

```C
size_t pa_memblock_get_length(const pa_memblock *b);
```

获取内存块的长度（以字节为单位）。

* `b`：指向要查询长度的内存块的指针。

返回内存块的长度。

```C
pa_mempool* pa_memblock_get_mempool(const pa_memblock *b);
```

获取内存块所属的内存池。

* `b`：指向要查询内存池的内存块的指针。

返回指向内存池的指针。

```C
bool pa_memblock_is_writable(pa_memblock *b);
```

检查是否可以直接修改这块内存.只有当`Ref Count == 1`且内存不是只读类型时，才返回 true。

* `b`：指向要检查的内存块的指针。

返回`true`表示可以直接修改，返回`false`表示不可以（可能是共享内存或只读内存）。

```C
bool pa_memblock_is_read_only(pa_memblock *b);
```

检查这块内存是否为只读类型。

* `b`：指向要检查的内存块的指针。

返回`true`表示是只读内存，返回`false`表示不是只读内存。

## 内存块片段pa_memchunk

`pa_memchunk`是Pulseaudio中用于表示`pa_memblock`片段的结构体。它定义在`<pulsecore/memchunk.h>`中。

`PulseAudio`为了避免内存拷贝，经常会在一个巨大的`memblock`（比如 64MB 的共享内存池）里存放几百个小的音频包。

* `Chunk A` 说：“我拥有这个 Block 的第 0 到 100 字节。”

* `Chunk B` 说：“我拥有同一个 Block 的第 1024 到 2048 字节。”

### pam_memchunk结构体

```C
struct pa_memchunk {
    pa_memblock *memblock; // 指向所属的内存块
    size_t index;          // 片段在内存块中的起始偏移
    size_t length;         // 片段的长度
};
```

### 生命周期管理

```C
void pa_memchunk_reset(pa_memchunk *c);
```

重置内存块片段，将其成员设置为默认值（`memblock`为`NULL`，`index`和`length`为0），如果`memblock`不为`NULL`，则会调用`pa_memblock_unref()`释放内存块。

* `c`：指向要重置的内存块片段的指针。

```C
pa_memchunk* pa_memchunk_init(pa_memchunk *c, pa_memblock *b, size_t index, size_t length);
```

初始化内存块片段，并自动增加内存块的引用计数。

* `c`：指向要初始化的内存块片段的指针。
* `b`：指向所属的内存块的指针。
* `index`：片段在内存块中的起始偏移。
* `length`：片段的长度。

### 写时复制

```C
void pa_memchunk_make_writable(pa_memchunk *c, size_t min_delta);
```

确保内存块片段是可写的。如果当前内存块的引用计数大于1，或者内存块是只读类型，则会创建一个新的内存块，并将片段的数据复制到新的内存块中，从而实现写时复制（Copy-On-Write）。

* `c`：指向要确保可写的内存块片段的指针。
* `min_delta`：指定在创建新内存块时，额外分配的字节数，以避免频繁的内存分配。通常设置为0即可。

### 数据拷贝

```C
pa_memchunk* pa_memchunk_memcpy(pa_memchunk *dst, pa_memchunk *src);
```

把`src`的数据拷贝到`dst`中,它会自动处理`dst->index`和`src->index`。它不是简单的内存拷贝，它是逻辑内容的拷贝。要求`dst->length`不小于`src->length`。

* `dst`：指向目标内存块片段的指针。
* `src`：指向源内存块片段的指针。

返回指向目标内存块片段的指针。

### 性能优化

```C
void pa_memchunk_will_need(const pa_memchunk *c);
```

通知内存管理器即将访问内存块片段的数据，允许预先加载数据以提高访问性能。

底层调用`madvise(MADV_WILLNEED)`系统调用。

## pa_memblockq

`pa_memblockq`是Pulseaudio中用于管理音频数据流的内存块队列。它定义在`<pulsecore/memblockq.h>`中。是一个支持“零拷贝”、“碎片化与合并”和“时光倒流（Rewind）”的字节流缓冲队列。

* 零拷贝：当`push`一个`chunk`进去时，它只是把这个`chunk`的引用（指针）挂到了链表末尾，没有发生`memcpy`。当`peek`时，它直接把内部`chunk`的指针返给你。
* 碎片化与合并：`push`了`10`次`100`字节的小包，`peek`时可以一次性把这`1000`字节的数据都取出来（合并）。`push`一个 500 字节的大包，`peek`时可以只取前`200`字节（碎片化）。

`pa_memblockq`不是线程安全的，调用者需要保证在单一线程中使用它。或者使用外部锁.

### 内部结构体

```C
struct pa_memblockq {
    pa_memchunk chunks[PA_MEMBLOCKQ_MAX_BLOCKS]; // 内存块片段数组
    size_t n_chunks;                             // 当前内存块片段数量
    size_t read_index;                           // 读取位置的索引 
    size_t write_index;                          // 写入位置的索引
    size_t length;                               // 当前队列中的数据长度
    size_t max_length;                           // 队列的最大长度
    pa_sample_spec sample_spec;                  // 音频采样规格
    pa_memblock *memblock;                       // 用于分配内存块的内存块
    // 其他成员...
};
```

内部维护了一个链表，链表的每个节点是一个`pa_memchunk`，表示一段连续的音频数据。通过维护读写索引，实现对音频数据的高效读写操作。

### 创建与销毁

```C
pa_memblockq* pa_memblockq_new(
    const char *name,          // 名字 (调试用)
    int64_t idx,               // 初始索引 (通常设为 0)
    size_t maxlength,          // 【关键】最大容量 (字节)。超过会导致 push 失败或旧数据被挤掉
    size_t tlength,            // 目标长度 (播放时用，录音/处理设 0 即可)
    const pa_sample_spec *ss,  // 【关键】采样格式 (用于生成静音)
    size_t prebuf,             // 预缓冲大小 (播放时用，设 0)
    size_t minreq,             // 最小请求 (播放时用，设 0)
    size_t maxrewind,          // 最大回卷长度 (不需要 rewind 就设 0)
    pa_memchunk *silence       // 静音块模板 (通常传 NULL，它会自动生成)
);
```

创建一个新的内存块队列。

* `name`：内存块队列的名称，用于调试和日志记录。
* `idx`：初始索引，通常设为`0`。
* `maxlength`：队列的最大长度（以字节为单位）。超过该长度时，`push`操作会失败或旧数据被挤掉。
* `tlength`：目标长度，通常用于播放时的缓冲管理。对于录音或处理，设为`0`即可。
* `ss`：音频采样规格，用于生成静音数据。
* `prebuf`：预缓冲大小，通常用于播放时的缓冲管理。对于录音或处理，设为`0`即可。
* `minreq`：最小请求大小，通常用于播放时的缓冲管理。对于录音或处理，设为`0`即可。
* `maxrewind`：最大回卷长度，表示允许回卷的最大字节数。不需要回卷时设为`0`。
* `silence`：静音块模板，如果传递`NULL`，则会根据采样规格自动生成静音数据。

返回一个指向新创建的`pa_memblockq`对象的指针。如果创建失败，返回`NULL`。

```C
void pa_memblockq_free(pa_memblockq *bq);
```

销毁内存块队列，释放内部所有未读数据的引用。

* `bq`：指向要销毁的内存块队列的指针。

### 写入

```C
int pa_memblockq_push(pa_memblockq *bq, const pa_memchunk *chunk);
```

将一个内存块片段写入内存块队列尾部。

* `bq`：指向目标内存块队列的指针。
* `chunk`：指向要写入的内存块片段的指针。

返回0表示写入成功，返回负值表示写入失败（例如队列已满）。

```C
int pa_memblockq_seek(pa_memblockq *bq, int64_t offset, pa_seek_mode_t seek, bool relative);
```

移动写指针位置。

* `bq`：指向目标内存块队列的指针。
* `offset`：偏移量，表示要移动的字节数。
* `seek`：寻址模式，可以是`PA_SEEK_SET`（从头开始）、`PA_SEEK_CUR`（从当前位置）或`PA_SEEK_END`（从末尾开始）。
* `relative`：如果为`true`，则表示偏移量是相对于当前写指针位置的；如果为`false`，则表示偏移量是绝对位置。

返回0表示移动成功，返回负值表示移动失败（例如超出范围）。

### 读取

```C
int pa_memblockq_peek(pa_memblockq *bq, pa_memchunk *chunk);
```

查看内存块队列头部的数据片段，但不移动读指针。

如果队列头部是一个`200`字节的碎片，它就给你`200`字节。

* `bq`：指向目标内存块队列的指针。
* `chunk`：指向用于存储读取数据片段的内存块片段的指针。

返回0表示读取成功，返回负值表示读取失败（例如队列为空）。

```C
int pa_memblockq_peek_fixed_size(pa_memblockq *bq, size_t block_size, pa_memchunk *chunk);
```

定长读，强行要求返回`block_size`长度的连续数据。

如果队列头部的碎片只有`960`字节，但需要`1024`.会自动申请一块新内存,从后面再借`64`字节，拼凑成`1024`字节填进去。

* `bq`：指向目标内存块队列的指针。
* `block_size`：要读取的固定块大小（以字节为单位）。
* `chunk`：指向用于存储读取数据片段的内存块片段的指针。

返回0表示读取成功，返回负值表示读取失败（例如队列中的数据不足）。

```C
void pa_memblockq_drop(pa_memblockq *bq, size_t length);
```

将内存块队列头部的指定长度的数据丢弃，并移动读指针。

* `bq`：指向目标内存块队列的指针。
* `length`：要丢弃的数据长度（以字节为单位）。

### 状态查询

```C
size_t pa_memblockq_get_length(pa_memblockq *bq);
```

获取当前内存块队列中的数据长度字节。

* `bq`：指向目标内存块队列的指针。

返回当前队列中的数据长度（以字节为单位）。

```C
void pa_memblockq_flush_write(pa_memblockq *bq, bool silence);
```

清空队列里的所有数据.
