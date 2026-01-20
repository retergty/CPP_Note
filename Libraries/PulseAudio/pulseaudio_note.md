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
