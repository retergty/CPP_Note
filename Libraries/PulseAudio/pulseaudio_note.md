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

## 异步消息队列

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
