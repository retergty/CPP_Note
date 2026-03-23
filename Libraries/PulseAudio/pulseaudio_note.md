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

模块通常在`usr/lib/pulse/modules/`目录下以`*.so`文件的形式存在，在`/etc/pulse/default.pa`配置文件中通过`load-module`命令加载，或者是通过命令行工具`pactl`动态加载，程序里`pa_module_load()`函数加载。

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

如果主线程阻塞，不会立即影响音频处理，因为音频数据传输是在IO线程中进行的。

### IO线程

`IO线程`是Pulseaudio中用于处理音频数据传输的专用线程。这是一个逻辑概念，本质上就是一个高优先级的线程。只有`pa_source`和`pa_sink`会创建IO线程，而`pa_source_output`和`pa_sink_input`则运行在它们所属的`pa_source`和`pa_sink`的IO线程中。

### thread_info

`thread_info`是Pulseaudio中用于描述音频流线程信息的结构体。它定义在`<pulsecore/thread-info.h>`中.

每个`pa`结构体，比如`pa_source`,`pa_sink`,`pa_sink_input`,`pa_source_output`等，都有一个`thread_info`成员变量.

只有IO线程可以操作thread_info,如果主线程需要修改，需要通过`pa_asyncmsgq_post()`发送消息到IO线程，由IO线程来修改.由消息处理回调修改，比如`pa_sink_process_msg()`.标准的修改逻辑pulseaudio已经给出。

### 模块开发流程

* 加载(init)模块，初始化相关数据结构，注册回调函数等。
* 监听(Callbacks)事件，如果回调函数被触发，执行相应的处理逻辑。
* 卸载(done)模块，释放资源，注销回调函数等。

### sample

`sample`是最原子的单位,代表了某一个声道在某一个时刻的振幅值.

它的数据类型取决于格式。

* `PA_SAMPLE_U8`：无符号8位整数，范围0-255，128为静音。
* `PA_SAMPLE_S16LE`：有符号16位整数，小端字节序，范围-32768到32767，0为静音。
* `PA_SAMPLE_S16BE`：有符号16位整数，大端字节序，范围-32768到32767，0为静音。
* `PA_SAMPLE_FLOAT32LE`: 32位浮点数，小端字节序，范围-1.0到1.0，0.0为静音。

### channel

`channel`表示音频流中的一个独立声道。例如，在立体声音频中，有两个通道：左声道和右声道。每个通道包含一系列的样本(sample)，这些样本表示了该通道在不同时间点的音频信号强度。

* 单声道(Mono)：只有一个通道，通常用于语音或单一音源。
* 立体声(Stereo)：有两个通道，分别为左声道和右声道，常用于音乐和多媒体内容。
* 5.1环绕声：包含六个通道，分别为左前、右前、中置、低音炮、左后和右后，常用于家庭影院系统。

### frame

`frame`是时间上的最小单位，它包含了该时刻所有通道的样本(sample)。

* 如果是单声道(S16LE),1 frame = 1 sample = 2 bytes.
* 如果是立体声(S16LE),1 frame = 2 samples = 4 bytes.

### sampling rate

`sampling rate`表示每秒钟采集的样本数量，单位为赫兹(Hz)。常见的采样率有：

* 44100 Hz：CD音质，常用于音乐播放。
* 48000 Hz：专业音频和视频制作常用的采样率。

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

## 按名字查找目标

```C
void* pa_namereg_get(
    pa_core *c,                 // 全局核心对象
    const char *name,           // 要查找的名字 (字符串)
    pa_namereg_type_t type      // 要查找的类型 (枚举)
);
```

在模块代码中使用`pa_namereg_get()`按名字查找目标对象。

参数

* `c`：指向Pulseaudio核心对象的指针，通常可以通过`m->core`获取。
* `name`：要查找的对象名称，通常是一个字符串。
* `type`：要查找的对象类型，使用`pa_namereg_type_t`枚举值，如`PA_NAMEREG_SINK`、`PA_NAMEREG_SOURCE`等。

返回指向目标对象的指针，如果未找到则返回`NULL`。

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

`pa_modargs_get_value_boolean`只有当参数不为数值类型时才返回错误码，找不到参数不会修改结果变量。

### 释放参数对象

使用`pa_modargs_free()`释放`pa_modargs`对象：

```C
void pa_modargs_free(pa_modargs *ma);
```

示例为：

```C
pa_modargs_free(ma);
```

## 采样规格pa_sample_spec

```CPP
typedef struct pa_sample_spec {
    pa_sample_format_t format; // 数据格式 (类型、位深、字节序)
    uint32_t rate;             // 采样率 (每秒多少帧)
    uint8_t channels;          // 通道数 (几条车道)
} pa_sample_spec;
```

表示采样规格的结构体，定义在`<pulse/sample.h>`中。

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

```C
struct pa_msgobject {
    pa_object parent; // 基类对象，包含引用计数等基本功能
    int (*process_msg)(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk); // 消息处理函数指针
};
```

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

## pa_thread

`pa_thread`是Pulseaudio中用于管理线程的结构体。它定义在`<pulsecore/thread.h>`中。是对操作系统底层线程的封装，提供了跨平台的线程创建、管理和同步功能。

### 创建线程

```C
pa_thread* pa_thread_new(
    const char *name,        // 线程名字 (top/htop 里能看到)
    pa_thread_func_t thread_func, // 线程主函数
    void *userdata           // 传给主函数的参数
);
```

创建并立即启动一个新线程。

* `name`：线程的名称，用于调试和日志记录。
* `thread_func`：线程的主函数，当线程启动时会调用该函数。
* `userdata`：传递给线程主函数的参数。

返回一个指向新创建的`pa_thread`对象的指针。如果创建失败，返回`NULL`。

`pa_thread_func_t`的回调定义为

```C
typedef void (*pa_thread_func_t)(void *userdata);
```

### 销毁线程

```C
void pa_thread_free(pa_thread *t);
```

等待线程结束并释放线程资源。

* `t`：指向要销毁的线程的指针。

### 实时优先级控制

```C
int pa_thread_make_realtime(int priority);
```

尝试将当前线程提升为实时优先级。

* `priority`：实时优先级的数值，范围通常为`1`到`99`，数值越大表示优先级越高。

返回`0`表示提升成功，返回负值表示提升失败（例如权限不足）。

### 辅助控制

```C
void pa_thread_yield(void);
```

让出当前线程的执行权，允许其他线程运行。

## pa_thread_mq

`pa_thread_mq`是Pulseaudio中用于在线程之间传递消息的结构体。它定义在`<pulsecore/thread-mq.h>`中。基于`pa_asyncmsgq`实现，提供了线程间通信的机制。

实现主函数与工作线程的双向通信，内部包含了两个`pa_asyncmsgq`.

* inq,主线程发送消息到工作线程
* outq,工作线程发送消息到主线程

### 创建线程消息队列

```C
int pa_thread_mq_init(
    pa_thread_mq *q,             // 你的结构体对象指针
    pa_mainloop_api *mainloop,   // 主线程的事件循环 (u->core->mainloop)
    pa_rtpoll *rtpoll            // 子线程的轮询器 (u->rtpoll)
);
```

构建双向通信管道，并建立物理连接。创建`inq`把他绑定在`rtpoll`上，创建`outq`把它绑定在`mainloop`上。

* `q`：指向要初始化的线程消息队列的指针。
* `mainloop`：指向主线程的事件循环的指针。
* `rtpoll`：指向工作线程的轮询器的指针。

返回`0`表示初始化成功，返回负值表示初始化失败。

### 销毁线程消息队列

```C
void pa_thread_mq_done(pa_thread_mq *q);
```

断开双向通信管道，释放相关资源。需要在工作线程`pa_thread_free()`后调用。

### 线程内激活

```C
void pa_thread_mq_install(pa_thread_mq *q);
```

将 q 注册到当前线程的 TLS (Thread Local Storage) 中。

这个函数必须在子线程内部调用。

### 发送消息

`pa_thread_mq`内部包含了两个`pa_asyncmsgq`，需要使用`pa_asyncmsgq`的接口进行消息发送和接收。

## pa_rtpoll

`pa_rtpoll`是Pulseaudio中用于管理事件轮询的结构体。它定义在`<pulsecore/rtpoll.h>`中。提供了跨平台的事件轮询机制，允许模块和组件在单一线程中处理多个I/O事件。

`pr_rtpool`使线程进入休眠等待唤醒的模式，线程会挂起，直到以下任意事件发生：

* 文件描述符 (FD)有信号
* 定时器到期
* 收到消息:主线程通过`pa_asyncmsgq`发来了指令.

### 创建

```C
pa_rtpoll *pa_rtpoll_new(void);
```

创建一个新的事件轮询对象。底层通常是`poll`或`epoll`。

返回一个指向新创建的`pa_rtpoll`对象的指针。如果创建失败，返回`NULL`。

### 销毁

```C
void pa_rtpoll_free(pa_rtpoll *p);
```

销毁事件轮询对象，释放相关资源。确保在销毁前，所有挂载在上面的 Item（如消息队列、Socket）都已经处理完毕或移除。

### 事件循环

```C
int pa_rtpoll_run(pa_rtpoll *p);
```

启动事件轮询循环，等待并处理事件。线程进入阻塞状态.

* `p`：指向要运行的事件轮询对象的指针。

返回值`<0`表示发生错误，返回值`>0`表示有事件发生，返回值`0`表示没有事件发生。

### 定时器控制

```C
void pa_rtpoll_set_timer_absolute(pa_rtpoll *p, pa_usec_t t);
```

设置绝对定时器，指定一个时间点，当系统时间达到该时间点时，轮询器会被唤醒。

* `p`：指向目标事件轮询对象的指针。
* `t`：绝对时间点，以微秒为单位。

通过`pa_rtclock_now()`获取当前时间，然后加上一个延迟值即可得到绝对时间点。

```C
void pa_rtpoll_set_timer_relative(pa_rtpoll *p, pa_usec_t t);
```

上一个函数的封装，设置相时间。

```C
void pa_rtpoll_set_timer_disabled(pa_rtpoll *p);
```

关闭定时器

## pa_source

`pa_source`是Pulseaudio中用于表示音频输入设备（如麦克风）的结构体。它定义在`<pulsecore/source.h>`中。

扮演生产者的角色，将音频数据采集并提供给系统中的其他组件使用。

### 状态机

`pa_source_state_t`定义了`pa_source`的各种状态：

* `PA_SOURCE_INIT`：初始化状态，源正在创建过程中。
* `PA_SOURCE_RUNNING`：运行状态，源正在采集音频数据。
* `PA_SOURCE_SUSPENDED`：暂停状态，源暂时停止采集音频数据。
* `PA_SOURCE_IDLE`：空闲状态，源没有音频数据可供。
* `PA_SOURCE_UNLINKED`：已断开状态，源已被卸载或断开连接。

`paulseaudio`会根据连接到`pa_source`的`source_output`数量和系统状态，在`RUNNING`和`IDLE`,`SUSPENDED`状态之间切换。

* 如果没有任何`source_output`连接到该`pa_source`，或者连接的所有`source_output`都处于`CORKED`状态，`pa_source`会进入`IDLE`状态.
* 如果有至少一个`source_output`连接且未`CORKED`，`pa_source`会进入`RUNNING`状态.
* 如果默认设备处于`IDLE`状态一段时间后，Pulseaudio会自动将其切换为`SUSPENDED`状态。

#### 例子

假设有一个APP连接到底层的ALSA Source设备.

* 初始状态：`SOURCE`处于`Suspended`状态.
* APP连接，`pa_stream_connect_record`,`APP`创建了一个`pa_source_output`连接到`SOURCE`，`SOURCE`进入`IDLE`,随后进入`RUNNING`状态，开始采集音频数据.
* APP调用`pa_stream_cork(TRUE)`,`SOURCE`进入`IDLE`状态，停止采集音频数据.
* 暂停了一段时间后，Pulseaudio自动将`SOURCE`切换为`SUSPENDED`状态。
* APP恢复录音，`pa_stream_cork(FALSE)`,`SOURCE`重新进入`IDLE`，随后进入`RUNNING`状态，继续采集音频数据.

### pa_source_new_data

`pa_source_new_data`是用于创建和初始化`pa_source`对象的辅助结构体。它定义在`<pulsecore/source.h>`中。

#### 常用字段

```C
char *name;                     // 源的名称 (唯一标识符)
pa_model *module;               // 所属模块的指针
pa_proplist *proplist;         // 属性列表 (key-value 对)
```

* `char *name;`：指定源的名称，必须是唯一的标识符。
* `pa_module *module;`：指向创建该源的模块的指针。通常就是`pa__init()`函数中的`m`参数。
* `pa_proplist *proplist;`：属性列表，用于存储源的各种属性信息，以键值对的形式存储。
* `char* driver;`：驱动名称，通常用于标识底层实现。通常设置为`__FILE__`宏的值，表示当前源代码文件名。

#### API函数

```C
void pa_source_new_data_init(pa_source_new_data *data);
```

初始化`pa_source_new_data`结构体，将其成员设置为默认值。

```C
void pa_source_new_data_set_name(pa_source_new_data *data, const char *name);
void pa_source_new_data_set_sample_spec(pa_source_new_data *data, const pa_sample_spec *ss);
void pa_source_new_data_set_channel_map(pa_source_new_data *data, const pa_channel_map *map);
void pa_source_new_data_set_alternate_sample_rate(pa_source_new_data *data, uint32_t rate);
```

有一系列的`set*`函数用于设置`pa_source_new_data`的各个成员，如名称、采样规格、通道映射和备用采样率等

```C
void pa_source_new_data_done(pa_source_new_data *data);
```

销毁`pa_source_new_data`结构体，释放相关资源。

### 创建pa_source对象

```C
pa_source* pa_source_new(
    pa_core *core,
    pa_source_new_data *data,
    pa_source_flags_t flags
);
```

创建一个新的`pa_source`对象。

* `core`：指向Pulseaudio核心对象的指针。
* `data`：指向已初始化的`pa_source_new_data`结构体的指针。
* `flags`：源的标志，指定源的行为和特性。
  * `PA_SOURCE_HARDWARE`：表示源是一个硬件设备。
  * `PA_SOURCE_LATENCY`：表示源支持延迟报告。可以根据`pa_source_output`的需求动态调整延迟。
  * `PA_SOURCE_DYNAMIC_LATENCY`：表示源的延迟是动态变化的。

返回一个指向新创建的`pa_source`对象的指针。如果创建失败，返回`NULL`。

### 配置

```C
void pa_source_set_asyncmsgq(pa_source *s, pa_asyncmsgq *q);
```

设置`pa_source`的异步消息队列，用于线程间通信。

* `s`：指向目标`pa_source`对象的指针。
* `q`：指向要设置的异步消息队列的指针。

```C
void pa_source_set_rtpoll(pa_source *s, pa_rtpoll *p);
```

绑定一个`pa_rtpoll`对象到`pa_source`，用于事件轮询.当 Source 需要处理音量变化或状态改变时，PA 会通过这个 rtpoll 唤醒该线程。

```C
void pa_source_set_max_rewind(pa_source *s, size_t nbytes);
```

设置`pa_source`的最大回卷长度。

* `s`：指向目标`pa_source`对象的指针。
* `nbytes`：最大回卷长度（以字节为单位）。

### 创建IO线程

```C
pa_thread* pa_thread_new(
    const char *name,        // 线程名字 (top/htop 里能看到)
    pa_thread_func_t thread_func, // 线程主函数
    void *userdata           // 传给主函数的参数
);
```

创建IO线程和创建普通线程没有区别，IO线程就是普通线程，这是一个编程模型，只有IO线程可以访问当前流的实时数据,也就是`thread_info`的数据.

### 上线启动

```C
void pa_source_put(pa_source *s);
```

正式将`pa_source`对象上线并启动采集音频数据的过程。进入`IDLE`状态或`RUNNING`状态，具体取决于源的配置和当前系统状态。触发`hook`,`PA_CORE_HOOK_SOURCE_PUT`.

### 推送音频数据

```C
void pa_source_post(pa_source *s, const pa_memchunk *chunk);
```

将`chunk`分发给所有连接的`source_output`。这是`pa_source`向系统提供音频数据的主要方式。

### 处理消息

```C
int pa_source_process_msg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk);
```

模块或其他组件通过消息机制与`pa_source`进行通信时调用的函数。处理各种消息代码，并执行相应的操作。如果模块通过`pa_asyncmsgq_post()`或`pa_asyncmsgq_send()`发送消息给`pa_source`，最终会调用这个函数进行处理。

在`pa_source`的实现中，通常会重写这个函数以处理特定的消息类型。

```C
int source_process_msg_cb(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk) {
    pa_source *s = PA_SOURCE(o);
    switch (code) {
        case MY_CUSTOM_MESSAGE:
            // 处理自定义消息
            break;
        default:
            return pa_source_process_msg(o, code, data, offset, chunk);
    }
    return 0;
}
```

然后通过修改`pa_source`对象的`parent.process_msg`指针来使用这个回调函数。

```C
u->source->parent.process_msg = my_source_process_msg;
```

### 切换状态

```C
int pa_source_suspend(
    pa_source *s,            // 目标 Source 对象
    bool suspend,            // true = 去睡觉 (挂起), false = 醒醒 (恢复)
    pa_suspend_cause_t cause // 挂起/恢复的原因 (非常重要!)
);
```

* `s`：指向目标`pa_source`对象的指针。
* `suspend`：如果为`true`，则表示将源挂起（暂停采集音频数据）；如果为`false`，则表示恢复源（继续采集音频数据）。
* `cause`：表示挂起或恢复的原因。

返回`0`表示操作成功，返回负值表示操作失败。

`pa_source`通过位掩码机制管理挂起原因，`source`可能会因为多个原因被挂起，只有当所有挂起原因都被清除后，`source`才会真正恢复运行。

常见的挂起原因包括：

* `PA_SUSPEND_USER`：用户请求挂起。
* `PA_SUSPEND_IDLE`：源处于空闲状态。
* `PA_SUSPEND_AVAILABILITY`：设备不可用。
* `PA_SUSPEND_INTERNAL`：内部原因。

```C
int pa_source_set_mute
(
    pa_source *s,    // 目标 Source 对象
    bool mute        // true = 静音, false = 取消静音
);
```

设置`pa_source`的静音状态。

```C
int pa_source_set_port(
    pa_source *s,          // 目标 Source 对象
    const char *port_name  // 端口名称
);
```

切换端口

### 销毁

```C
void pa_source_unlink(pa_source *s);
```

将`pa_source`对象下线。触发`hook`,`PA_CORE_HOOK_SOURCE_UNLINK`.

```C
void pa_source_unlink(pa_source *s);
```

销毁`pa_source`对象，释放相关资源。确保在销毁前，所有连接的`source_output`都已经断开连接。

### 设置回调函数

直接给`u->source`赋值即可

```C
// 状态改变回调 (IDLE <-> RUNNING)
s->set_state = my_set_state_cb; 

// 当请求的laetency发生变化时调用
// 比如pa_source_output连接上来或断开时,pa_source_output有时会设置请求的延迟.
s->update_requested_latency = my_update_latency_cb;
```

## pa_source_output

`pa_source_output`接收`pa_source`推送的音频数据，并将其传递给下游处理模块或组件。它定义在`<pulsecore/source-output.h>`中。

### 内部结构

内部分为三个部分

* 重采样器`pa_resampler`：用于将输入音频数据转换为目标采样规格。
* 缓冲队列`pa_memblockq`：用于存储和管理音频数据流。
* 状态机：管理`pa_source_output`的生命周期和状态转换。

### 状态机

`pa_source_output_state_t`定义了`pa_source_output`的各种状态：

* `PA_SOURCE_OUTPUT_INIT`：初始化状态，源输出正在创建过程中。
* `PA_SOURCE_OUTPUT_RUNNING`：运行状态，源输出正在传输音频数据。
* `PA_SOURCE_OUTPUT_CORKED`：暂停状态，源输出暂时停止传输音频数据。
* `PA_SOURCE_OUTPUT_UNLINKED`：已断开状态，源输出已被卸载或断开连接。

### pa_source_output_new_data

```C
void pa_source_output_new_data_init(pa_source_output_new_data *data);
```

配置初始化结构体

```C
void pa_source_output_new_data_set_source(
    pa_source_output_new_data *data, 
    pa_source *s, 
    bool save,   // 是否保存这个选择（用于下次自动恢复）
    bool fix     // 是否强制绑定（禁止被移动到其他 Source）
);
```

设置关联的`pa_source`对象。

```C
void pa_source_output_new_data_set_sample_spec(pa_source_output_new_data *data, const pa_sample_spec *ss);
```

设置数据格式.

* `data`：指向要设置的`pa_source_output_new_data`结构体的指针。
* `ss`：指向要设置的采样规格的指针。

### 创建pa_source_output对象

```C
int pa_source_output_new(
    pa_source_output **o,   // [输出] 返回创建好的对象指针
    pa_core *core, 
    pa_source_output_new_data *data, 
    pa_source_output_flags_t flags
);
```

* `o`：指向用于存储新创建的`pa_source_output`对象指针的指针。
* `core`：指向Pulseaudio核心对象的指针。
* `data`：指向已初始化的`pa_source_output_new_data`结构体的指针。
* `flags`：源输出的标志，指定源输出的行为
  * `PA_SOURCE_OUTPUT_VARIABLE_RATE`：表示源输出支持可变采样率。
  * `PA_SOURCE_OUTPUT_DONT_MOVE`：表示源输出不允许被移动到其他源。

返回`0`表示创建成功，返回负值表示创建失败。

### 设置回调函数

挂上hook函数,设置`u->source_output`的回调函数指针即可.

```C
void (*push)(pa_source_output *o, const pa_memchunk *chunk);
```

运行在目标`pa_source`的线程上下文中。当`pa_source`推送音频数据时调用该函数。负责处理和传递音频数据。

* `o`：指向目标`pa_source_output`对象的指针。
* `chunk`：指向要处理的音频数据片段的指针。

```C
void (*kill)(pa_source_output *o);
```

运行在主线程上下文中。当`pa_source`被强制删除时调用.

* `o`：指向目标`pa_source_output`对象的指针。

```C
void (*state_change)(pa_source_output *o, pa_source_output_state_t state);
```

运行在主线程上下文中。当`pa_source_output`的状态发生变化时调用该函数。负责处理状态变化的逻辑。

* `o`：指向目标`pa_source_output`对象的指针。
* `state`：新的状态值，表示`pa_source_output`的当前状态。

```C
void (*attach)(pa_source_output *o);
```

运行在主线程上下文中。当`pa_source_output`被连接到`pa_source`时调用该函数。负责处理连接逻辑。

* `o`：指向目标`pa_source_output`对象的指针。

```C
void (*detach)(pa_source_output *o);
```

运行在主线程上下文中。当`pa_source_output`从`pa_source`断开连接时调用该函数。负责处理断开连接的逻辑。

```C
void (*moved)(pa_source_output *o);
```

运行在主线程上下文中。当`pa_source_output`被移动到另一个`pa_source`时调用该函数。负责处理移动逻辑。

### 上线

```C
void pa_source_output_put(pa_source_output *o);
```

将`pa_source_output`对象上线并开始接收音频数据。触发`hook`,`PA_CORE_HOOK_SOURCE_OUTPUT_PUT`.

### 销毁

```C
void pa_source_output_unlink(pa_source_output *o);
```

断开连接，将`pa_source_output`对象下线。触发`hook`,`PA_CORE_HOOK_SOURCE_OUTPUT_UNLINK`.

```C
void pa_source_output_unref(pa_source_output *o);
```

销毁`pa_source_output`对象，释放相关资源。确保在销毁前，已经调用了`pa_source_output_unlink()`断开连接。

### pa_resampler重采样器

`pa_resampler`是Pulseaudio中用于音频重采样的结构体。它定义在`<pulsecore/resampler.h>`中。用于在不同采样率之间转换音频数据。

如果`pa_source_output`的采样规格与连接的`pa_source`的采样规格不匹配，Pulseaudio会自动创建一个`pa_resampler`对象来进行重采样。

当`pa_source`调用`pa_source_post`分发数据时，会先在`pa_source_output_push`把数据送入`pa_resampler_run()`进行重采样，然后再放入`pa_memblockq`缓冲队列中。

如果需要支持变采样率，还需要在创建`pa_source_output`时设置`PA_SOURCE_OUTPUT_VARIABLE_RATE`标志。

`move`后需要手动重建`pa_resampler`,在PA的主线程中，

```C
if (u->resampler) {
            pa_resampler_free(u->resampler);
            u->resampler = NULL;
        }
// 如果需要，重新初始化重采样器
u->resampler = pa_resampler_new(..., &u->my_spec, ...);
```

### 移动pa_source_output

```C
int pa_source_output_move_to(
    pa_source_output *o,   // 目标 Source Output 对象
    pa_source *new_source  // 目标 Source 对象
);
```

将`pa_source_output`对象移动到另一个`pa_source`对象。

通常在主线程上下文中调用。当一个设备插入到系统时，可能需要将现有的`pa_source_output`移动到新的`pa_source`上。

但是，需要删除旧的`pa_resampler`,删除旧的`pa_memblockq`,重新创建新的`pa_resampler`和`pa_memblockq`，注意并发问题.

可以注册`moving`回调函数，在这个函数里面进行操作。

### 自动重混音

如果`pa_source_output`的通道映射与连接的`pa_source`的通道映射不匹配，Pulseaudio会自动进行重混音处理。它会创建一个内部的重混音器来调整音频数据的通道布局，以确保数据能够正确地传递和处理。

只需要设置`pa_source_output_new_data_set_sample_spec()`和`pa_source_output_new_data_set_channel_map()`，PA会自动处理重混音的逻辑。

如果不设置`pa_source_output_new_data_set_channel_map()`，PA会默认使用`pa_channel_map_init_auto()`生成一个通道映射，这个映射会根据采样规格和系统配置自动确定通道布局。

#### 底层处理

第一步：处理 Channel Map（只看通道数量）
PulseAudio 核心引擎在检查你的 data 配置时，发现你没有设置 Channel Map，它只比较通道数：

```C
// 底层逻辑
if (data->sample_spec.channels == data->source->sample_spec.channels) {
    // 只要数量一样（比如都是 2），管你格式是什么，直接拷贝底层的映射表！
    data->channel_map = data->source->channel_map; 
}
```

第二步：处理Format和sample_rate

`Channel Map`补全之后，`PulseAudio`接着往下检查`sample_spec`的具体内容。这时候它发现了差异：

`PulseAudio`就会触发我们在前面讨论过的重采样器（Resampler）机制：它会在底层 `Source` 和你的`source_output`之间安插一个`pa_resampler`。这个`resampler`会负责把底层`Source`的采样率和格式转换成你在`source_output_new_data`里设置的采样率和格式。

## 常见场景

### pa_source_output设置latency

当一个`pa_source_output`连接到一个`pa_source`时，它可能会请求特定的延迟（latency）值。这通常通过调用`pa_source_output_set_requested_latency()`函数来实现。

```C
int pa_source_output_set_requested_latency(pa_source_output *o, pa_usec_t latency);
int pa_source_output_set_requested_latency_within_thread(pa_source_output *o, pa_usec_t latency);
```

第一个函数用于在主线程上下文中设置请求的延迟，而第二个函数用于在`pa_source`的线程上下文中设置请求的延迟。实际上就是第一个函数给IO线程发消息，而第二个函数直接修改`thread_info`的数据.

* `o`：指向目标`pa_source_output`对象的指针。
* `latency`：请求的延迟值，以微秒为单位。

返回`0`表示设置成功，返回负值表示设置失败。

当`pa_source_output`请求延迟时，`pa_source`会调用其`update_requested_latency`回调函数，以便根据新的请求延迟调整其内部缓冲区和处理逻辑。
