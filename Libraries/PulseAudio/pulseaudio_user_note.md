# PulseAudio使用笔记

使用`pulseaudio`库进行音频处理，使用的是`libpulse`的稳定API，通过`Socket`或者是`IPC`进行通信。

## 核心对象

### pa_mainloop

`pa_mainloop`是PulseAudio的主事件循环对象，负责处理异步事件和回调。它管理所有的I/O事件、定时器和信号处理。

### pa_context

`pa_context`是与PulseAudio服务器通信的上下文对象。它用于管理与服务器的连接状态.

### pa_stream

`pa_stream`是音频数据流对象，表示一个音频流，可以用于播放或录制音频数据。它与具体的音频设备（`sink`或`source`）关联。

## libpulse-simple

`libpulse-simple`是一个简化的API，适用于简单的音频播放和录制任务。它提供了更易用的接口，适合初学者和简单应用。

### 常用函数

#### pa_simple_new

```C
pa_simple* pa_simple_new(
    const char *server,             // 1. 服务器地址
    const char *name,               // 2. 客户端应用名
    pa_stream_direction_t dir,      // 3. 数据流向
    const char *dev,                // 4. 目标设备名
    const char *stream_name,        // 5. 流描述名
    const pa_sample_spec *ss,       // 6. 采样规格
    const pa_channel_map *map,      // 7. 通道映射
    const pa_buffer_attr *attr,     // 8. 缓冲属性 (关键!)
    int *error                      // 9. 错误码返回
);
```

创建一个新的`pa_simple`对象，用于音频播放或录制。本质上是在`PA`的内核里创建了一个`pa_source_output`或`pa_sink_input`对象。

* `server`：指定PulseAudio服务器地址，通常为`NULL`表示本地服务器。
* `name`：客户端应用的名称，用于标识应用。会显示在`pactl list clients`中。
* `dir`：数据流向，`PA_STREAM_PLAYBACK`表示播放，`PA_STREAM_RECORD`表示录制。
* `dev`：目标设备名称，就是`pa_source`或`pa_sink`的名字，通常为`NULL`表示默认设备。
* `stream_name`：流的描述名称，用于标识音频流。会显示在`pactl list sink-inputs`或`pactl list source-outputs`中。
* `ss`：采样规格，定义音频数据的格式，如采样率、通道数和样本格式。
* `map`：通道映射，定义音频通道的布局，通常为`NULL`表示默认布局。
* `attr`：缓冲属性，定义音频缓冲区的大小和行为，关键参数。决定了**延迟**.
* `error`：用于返回错误码的指针，如果函数调用失败，可以通过该指针获取错误信息。

返回值为指向新创建的`pa_simple`对象的指针，如果创建失败则返回`NULL`。

#### pa_simple_read

```C
int pa_simple_read(
    pa_simple *s, 
    void *data, 
    size_t bytes, 
    int *error
);
```

从`pa_simple`对象中读取音频数据，适用于录制音频，这个过程是阻塞的，如果缓冲区里没有足够的数据，会等待直到数据可用。缓冲区大小就是`att`设置的大小。

* `s`：指向`pa_simple`对象的指针。
* `data`：指向用于存储读取音频数据的缓冲区。
* `bytes`：要读取的字节数。
* `error`：用于返回错误码的指针。

#### pa_simple_write

```C
int pa_simple_write(
    pa_simple *s, 
    const void *data, 
    size_t bytes, 
    int *error
);
```

向`pa_simple`对象写入音频数据，适用于播放音频，这个过程是阻塞的，如果缓冲区满了，会等待直到有空间可用。缓冲区大小就是`att`设置的大小。

* `s`：指向`pa_simple`对象的指针。
* `data`：指向包含要写入音频数据的缓冲区。
* `bytes`：要写入的字节数。
* `error`：用于返回错误码的指针。

#### pa_simple_get_latency

```C
pa_usec_t pa_simple_get_latency(pa_simple *s, int *error);
```

获取当前`pa_simple`对象的延迟，以微秒为单位。延迟是指从音频数据被写入到实际播放之间的时间，或者从音频数据被录制到被读取之间的时间。对于播放，不仅包含缓冲区的延迟，还包括`sink`报告的延迟。对于录音，包含已经在缓冲区里没被读走的数据时长+硬件捕获延迟。

* `s`：指向`pa_simple`对象的指针。
* `error`：用于返回错误码的指针。

#### pa_simple_drain

```C
int pa_simple_drain(pa_simple *s, int *error);
```

仅限播放，阻塞直到所有写入的音频数据都被播放完毕。确保在关闭`pa_simple`对象之前，所有音频数据都已经被处理和播放。

* `s`：指向`pa_simple`对象的指针。
* `error`：用于返回错误码的指针。

#### pa_simple_flush

```C
int pa_simple_flush(pa_simple *s, int *error);
```

立即丢弃缓冲区中所有未播放或未读取的音频数据。适用于需要快速停止音频处理的场景。

* `s`：指向`pa_simple`对象的指针。
* `error`：用于返回错误码的指针。

#### pa_simple_free

```C
void pa_simple_free(pa_simple *s);
```

断开并释放`pa_simple`对象及其相关资源。

* `s`：指向`pa_simple`对象的指针。

#### pa_strerror

```C
const char* pa_strerror(int error);
```

把错误码转化为可读的错误信息字符串，便于调试和日志记录。

* `error`：错误码。

## libpulse

`libpulse`是PulseAudio的核心库，提供了更全面和复杂的API，适用于需要更高控制和定制化的音频应用。

### 主循环pa_threaded_mainloop

`pa_threaded_mainloop`将`pulseaudio`的事件循环封装在独立的`POSIX`线程中运行.

#### 生命周期管理

```C
pa_threaded_mainloop* pa_threaded_mainloop_new(void);
```

创建一个新的`pa_threaded_mainloop`对象。

返回指向新创建的`pa_threaded_mainloop`对象的指针，如果创建失败则返回`NULL`。

```C
void pa_threaded_mainloop_free(pa_threaded_mainloop* m);
```

释放`pa_threaded_mainloop`对象及其相关资源。

* `m`：指向`pa_threaded_mainloop`对象的指针。

#### 线程控制

```C
int pa_threaded_mainloop_start(pa_threaded_mainloop* m);
```

启动后台线程，开始运行事件循环。类似于进入一个`epoll`循环。

返回`0`表示成功，非`0`表示失败。

```C
void pa_threaded_mainloop_stop(pa_threaded_mainloop* m);
```

向后台线程发送停止信号，阻塞等待直到停止事件循环并终止线程。

#### 同步控制

```C
void pa_threaded_mainloop_lock(pa_threaded_mainloop* m);
void pa_threaded_mainloop_unlock(pa_threaded_mainloop* m);
```

封装了`pthread_mutex_lock`和`pthread_mutex_unlock`，用于保护对共享资源的访问。

如果不在`PA`的回调函数中，但需要访问与`mainloop`相关的资源时，比如`pa_context`,`pa_stream`等，必须先锁定`mainloop`，操作完成后再解锁。

```C
void pa_threaded_mainloop_wait(pa_threaded_mainloop* m);
```

条件变量，释放`mainloop`的锁并阻塞等待，直到收到信号后重新获取锁并返回。

```C
void pa_threaded_mainloop_signal(pa_threaded_mainloop* m, int wait_for_accept);
```

向等待的线程发送信号，唤醒它们继续执行。通常是在回调函数中调用，以通知等待的线程某个事件已经发生。

* `m`：指向`pa_threaded_mainloop`对象的指针。
* `wait_for_accept`：
  * `0`：发送信号后立即返回。
  * `1`：发送信号后阻塞等待,直到对方调用了`pa_threaded_mainloop_accept()`.

#### 连接与接口

```C
pa_mainloop_api* pa_threaded_mainloop_get_api(pa_threaded_mainloop* m);
```

获取与`pa_threaded_mainloop`关联的`pa_mainloop_api`接口，用于创建和管理`pa_context`和`pa_stream`等对象。

### 操作句柄pa_operation

pa_operation 是 PulseAudio 异步编程模型中重要的控制句柄，每个控制操作都会返回一个 pa_operation 对象。这个对象可以

* 查询状态(get_state).
* 取消操作(cancel).
* 等待完成(sync).

`pa_operation`对象的状态如下

* `PA_OPERATION_RUNNING`：操作正在进行中，指令已经发过去了，但还没有收到服务器的响应。
* `PA_OPERATION_DONE`：操作已经完成，服务器已经处理完请求并返回了结果。
* `PA_OPERATION_CANCELED`：操作被取消，客户端调用了取消函数

可以用函数检查状态

```C
pa_operation_state_t s = pa_operation_get_state(o);
```

每次都需要释放`pa_operation`对象

```C
void pa_operation_unref(pa_operation *o);
```

### 连接层pa_context

`pa_context`代表`APP`与`PulseAudio`服务器之间的连接上下文。

`pulseaudio`使用引用计数来管理`Context`的内存。

#### 生命周期管理

```C
pa_context* pa_context_new(pa_mainloop_api *mainloop, const char *name);
```

创建一个新的`pa_context`对象，用于与PulseAudio服务器通信。

* `mainloop`：指向`pa_mainloop_api`接口的指针，通常通过`pa_threaded_mainloop_get_api()`获取。
* `name`：客户端应用的名称，用于标识应用。

返回指向新创建的`pa_context`对象的指针，如果创建失败则返回`NULL`。

```C
void pa_context_unref(pa_context *c);
```

减少`pa_context`对象的引用计数，当引用计数为零时释放对象及其相关资源。

* `c`：指向`pa_context`对象的指针。

```C
int pa_context_connect(
    pa_context *c, 
    const char *server, 
    pa_context_flags_t flags, 
    const pa_spawn_api *api
);
```

连接到PulseAudio服务器。

* `c`：指向`pa_context`对象的指针。
* `server`：服务器地址，通常为`NULL`表示本地服务器。
* `flags`：连接标志，控制连接行为。
  * `PA_CONTEXT_NOAUTOSPAWN`：如果服务器未运行，则不自动启动它。
  * `PA_CONTEXT_NOFAIL`：如果连接失败，不返回错误，而是保持未连接状态。
* `api`：用于进程间通信的API，通常为`NULL`。

返回`0`表示成功，非`0`表示失败。

```C
void pa_context_disconnect(pa_context *c);
```

断开与PulseAudio服务器的连接。

* `c`：指向`pa_context`对象的指针。

#### 状态机机制

```C
pa_context_state_t pa_context_get_state(const pa_context *c);
```

* 获取`pa_context`对象的当前状态。

状态为

* `PA_CONTEXT_UNCONNECTED`：未连接状态。
* `PA_CONTEXT_CONNECTING`：正在连接状态。
* `PA_CONTEXT_AUTHORIZING`：正在授权状态。
* `PA_CONTEXT_SETTING_NAME`：正在设置名称状态。
* `PA_CONTEXT_READY`：已连接并准备好状态。
* `PA_CONTEXT_FAILED`：连接失败状态。
* `PA_CONTEXT_TERMINATED`：连接已终止状态。

```C
void pa_context_set_state_callback(pa_context *c, pa_context_notify_cb_t cb, void *userdata);
```

通过设置回调函数监控`pa_context`状态变化。当状态发生变化时，回调函数会被调用。

#### 操作与控制

`pa_context`提供了丰富的操作接口来控制服务器,它们的共同特点是：异步，返回`pa_operation*`.

通用模式是

* `Request` $\to$ `pa_operation*` $\to$ `Callback` $\to$ `Notify Done`.

自省操作

* `pa_context_get_sink_info_list`列出所有的`sink`.
* `pa_context_get_source_info_list`列出所有的`source`.
* `pa_context_get_server_info`获取服务器信息.

控制操作

* `pa_context_set_sink_volume_by_name`设置指定`sink`的音量.
* `pa_context_set_default_sink`设置默认`sink`.
* `pa_context_suspend_sink_by_name`挂起指定`sink`.
* `pa_context_move_sink_input_by_name`移动`sink input`到指定的`sink`.
* `pa_context_set_source_mute_by_name`设置指定`source`的静音状态.
* `pa_context_kill_client`终止指定的客户端连接.
* `pa_context_load_module`加载指定的模块.
* `pa_context_unload_module`卸载指定的模块.
* `pa_context_cork_sink_input`暂停指定的`sink input`.
* `pa_context_set_sink_input_volume`设置指定`sink input`的音量.
* `pa_context_set_source_output_volume`设置指定`source output`的音量.

#### 事件订阅

```C
pa_operation* pa_context_subscribe(
    pa_context *c, 
    pa_subscription_mask_t m, 
    pa_context_success_cb_t cb, 
    void *userdata
);
```

订阅服务器事件通知，以便在特定事件发生时接收回调。

* `c`：指向`pa_context`对象的指针。
* `m`：订阅掩码，指定要订阅的事件类型。
  * `PA_SUBSCRIPTION_MASK_SINK`：与`sink`相关的事件。
  * `PA_SUBSCRIPTION_MASK_SOURCE`：与`source`相关的事件。
  * `PA_SUBSCRIPTION_MASK_SINK_INPUT`：与`sink input`相关的事件。
  * `PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT`：与`source output`相关的事件。
  * `PA_SUBSCRIPTION_MASK_MODULE`：与模块相关的事件。
  * `PA_SUBSCRIPTION_MASK_CLIENT`：与客户端相关的事件。
  * `PA_SUBSCRIPTION_MASK_SERVER`：与服务器相关的事件。
  * `PA_SUBSCRIPTION_MASK_AUTOLOAD`：与自动加载相关的事件.
* `cb`：回调函数，当订阅的事件发生时调用。
* `userdata`：传递给回调函数的用户数据指针。

```C
void pa_context_set_subscribe_callback(pa_context *c, pa_context_subscribe_cb_t cb, void *userdata);
```

回调函数类型.

### 数据层pa_stream

`pa_stream`表示一个音频数据流，可以用于播放或录制音频数据。

#### 状态机

```C
pa_stream_state_t pa_stream_get_state(const pa_stream *s);
```

获取`pa_stream`对象的当前状态。

* 状态为
  * `PA_STREAM_UNCONNECTED`：未连接状态。
  * `PA_STREAM_CREATING`：正在创建状态。
  * `PA_STREAM_READY`：已连接并准备好状态。
  * `PA_STREAM_FAILED`：连接失败状态。
  * `PA_STREAM_TERMINATED`：连接已终止状态。

#### 关键配置结构体

* `pa_sample_spec`定义音频数据的格式.比如采样率、通道数和样本格式.
* `pa_buffer_attr`定义音频缓冲区的大小和行为，关键参数，决定了**延迟**.
* `pa_channel_map`定义音频通道的布局.

`pa_buffer_attr`结构体的关键字段：

```C
typedef struct pa_buffer_attr {
    uint32_t maxlength; // 服务器端缓冲区的最大硬限制 (bytes)
    uint32_t tlength;   // [播放专用] Target Length (目标延迟)
    uint32_t prebuf;    // [播放专用] Pre-buffering (起播阈值)
    uint32_t minreq;    // [播放专用] Minimum Request (最小请求块)
    uint32_t fragsize;  // [录音专用] Fragment Size (中断碎片大小)
} pa_buffer_attr;
```

* `maxlength`：服务器端缓冲区的最大硬限制，单位为字节。

对于播放

* `tlength`：目标延迟，表示希望缓冲区保持的音频数据长度，单位为字节。影响播放的延迟时间。
* `minreq`: 最小请求块，表示当缓冲区中的可用数据少于该值时，客户端应请求更多数据，单位为字节。
* `prebuf`：起播阈值，表示在开始播放之前，缓冲区中必须至少有这么多数据，单位为字节。通常设置为`tlength`的一半。

对于录音

* `fragsize`：中断碎片大小，表示每次录音中断时传输的数据块大小，单位为字节。影响录音的延迟时间。比如希望20ms就处理一次数据，就设置为`pa_usec_to_bytes(20000)`.

#### 生命周期管理

```C
pa_stream* pa_stream_new(
    pa_context *c,              // 必须依附于一个已经连接的 Context
    const char *name,           // 流的名字 (显示在 pavucontrol 里，如 "Robot Speech")
    const pa_sample_spec *ss,   // 采样规格 (S16LE, 44100, 2ch...)
    const pa_channel_map *map   // 通道映射 (NULL = 默认)
);
```

创建一个新的`pa_stream`对象，用于音频播放或录制。

* `c`：指向已经连接的`pa_context`对象的指针。
* `name`：流的名称，用于标识音频流。会显示在`pactl list sink-inputs`或`pactl list source-outputs`中。
* `ss`：采样规格，定义音频数据的格式，如采样率、通道数和样本格式。
* `map`：通道映射，定义音频通道的布局，通常为`NULL`表示默认布局。

返回指向新创建的`pa_stream`对象的指针，如果创建失败则返回`NULL`。

```C
void pa_stream_unref(pa_stream *s);
```

减少`pa_stream`对象的引用计数，当引用计数为零时释放对象及其相关资源。

#### 设置回调函数

```C
// 1. 状态变了 (CREATING -> READY)
pa_stream_set_state_callback(s, state_cb, u);

// 2. [录音] 有新数据到了
pa_stream_set_read_callback(s, read_cb, u);

// 3. [播放] 缓冲区空了，需要数据
pa_stream_set_write_callback(s, write_cb, u);

// 4. [异常] 爆音/由于数据供应不足导致断流
pa_stream_set_underflow_callback(s, underflow_cb, u);

// 5. [异常] 缓冲区溢出 (你读得太慢了)
pa_stream_set_overflow_callback(s, overflow_cb, u);

// 6. [高级] 延迟更新了 (当 Server 调整了内部时钟)
pa_stream_set_latency_update_callback(s, latency_cb, u);
```

设置各种回调函数，以便在特定事件发生时接收通知和处理。

#### 连接与配置

```C
int pa_stream_connect_playback(
    pa_stream *s,               // [必填] 已经 new 好的流对象
    const char *dev,            // [选填] 目标设备名
    const pa_buffer_attr *attr, // [选填] 缓冲控制 (延迟的核心)
    pa_stream_flags_t flags,    // [选填] 行为标志位
    const pa_cvolume *volume,   // [选填] 初始音量
    pa_stream *sync_stream      // [选填] 同步流 (通常 NULL)
);
```

连接`pa_stream`对象以进行音频播放。

* `s`：指向`pa_stream`对象的指针。
* `dev`：目标设备名称，就是`sink`的名字，通常为`NULL`表示默认设备。
* `attr`：缓冲属性，定义音频缓冲区的大小和行为，关键参数。决定了**延迟**.
* `flags`：行为标志位，控制连接行为。
* `volume`：初始音量，如果为`NULL`，则使用默认音量,继承当前 Sink 的音量.
* `sync_stream`：同步流，通常为`NULL`,用于将当前流与另一个流的时间轴强行对齐.

返回`0`表示成功，非`0`表示失败。

```C
int pa_stream_connect_record(
    pa_stream *s,               // [必填] 流对象
    const char *dev,            // [选填] 目标设备名
    const pa_buffer_attr *attr, // [选填] 缓冲控制 (fragsize 核心)
    pa_stream_flags_t flags     // [选填] 行为标志位
);
```

连接`pa_stream`对象以进行音频录制。

* `s`：指向`pa_stream`对象的指针。
* `dev`：目标设备名称，就是`source`的名字，通常为`NULL`表示默认设备。
* `attr`：缓冲属性，定义音频缓冲区的大小和行为，关键参数。决定了**延迟**.
* `flags`：行为标志位，控制连接行为。

返回`0`表示成功，非`0`表示失败。

注意，这两个函数是异步的，需要等待`stream_state_callback`变为`PA_STREAM_READY`后，才能进行读写操作.

`flags`常用选项：

* `PA_STREAM_ADJUST_LATENCY`:尽量满足我在`attr->tlength`或`attr->fragsize`里设定的要求，如果需要，重新配置底层硬件的 Buffer 大小。
* `PA_STREAM_INTERPOLATE_TIMING`:启用时间戳插值机制，提高延迟估算的准确性，但会增加CPU开销。
* `PA_STREAM_AUTO_TIMING_UPDATE`:在后台更新时间戳，不需要手动请求更新时间戳.
* `PA_STREAM_INTERPOLATE_TIMING`:启用时间戳插值机制，提高延迟估算的准确性，如果你需要调用`pa_stream_get_time()`或 `pa_stream_get_latency()`来做音视频同步或回声消除，必须加这个标志.
* `PA_STREAM_START_CORKED`连接建立后，流处于 PAUSED (CORKED) 状态，需要手动调用`pa_stream_cork()`来启动流.
* `PA_STREAM_START_UNMUTED/MUTED`强制流在连接后处于 UNMUTED/MUTED 状态，忽略当前 Sink/Source 的静音状态.
* `PA_STREAM_NO_REMIX_CHANNELS`禁止自动混音通道数，如果采样规格和目标设备不匹配，连接会失败.
* `PA_STREAM_FIX_FORMAT / FIX_RATE / FIX_CHANNELS`：强制采样规格的格式/采样率/通道数必须和目标设备匹配，否则连接会失败.

#### 读写音频数据

```C
int pa_stream_write(
    pa_stream *s,
    const void *data,
    size_t bytes,
    pa_free_cb_t free_cb,
    int64_t offset,
    pa_seek_mode_t seek
);
```

向`pa_stream`对象写入音频数据，适用于播放音频。通常在`write_callback`中调用。

* `s`：指向`pa_stream`对象的指针。
* `data`：指向包含要写入音频数据的缓冲区。
* `bytes`：要写入的字节数。
* `free_cb`：数据释放回调函数，当数据不再需要时调用。如果为`NULL`，则表示数据在调用时已经被复制，不需要释放。
* `offset`：数据写入的偏移位置，通常为`PA_SEEK_RELATIVE`.
* `seek`：偏移模式，通常为`PA_SEEK_RELATIVE`.

返回`0`表示成功，非`0`表示失败。

```C
int pa_stream_begin_write(
    pa_stream *s,
    void **data,
    size_t *nbytes
);
```

开始写入音频数据，获取一个指向可写缓冲区的指针。适用于播放音频。减少数据复制开销。之后自行填充数据，然后调用`pa_stream_write()`提交。

* `s`：指向`pa_stream`对象的指针。
* `data`：指向用于存储写入音频数据的缓冲区
* `nbytes`：输入时表示请求的字节数，返回时表示实际可写的字节数。

返回`0`表示成功，非`0`表示失败。

```C
int pa_stream_peek(pa_stream *s, const void **data, size_t *length);
```

获取指向可读音频数据的指针。

* `s`：指向`pa_stream`对象的指针。
* `data`：指向用于存储读取音频数据的缓冲区指针。
* `length`：指向存储可读字节数的指针。

返回`0`表示成功，非`0`表示失败。

还需要根据data和length判断数据

* `data!= NULL && *length > 0`：有数据可读.
* `data== NULL && *length == 0`：没有数据可读，但流还在运行,可能是网络丢包等，应该在`buffer`里填入`length`长度的 0 (静音)。
* `data== NULL && *length == 0`: 无数据，退出等待下一次回调

```C
int pa_stream_drop(pa_stream *s);
```

丢弃刚刚通过`pa_stream_peek()`获取的音频数据，表示已经处理完这些数据，可以释放缓冲区。

* `s`：指向`pa_stream`对象的指针。

返回`0`表示成功，非`0`表示失败。

#### 控制流

```C
/*
 * s: 流对象
 * b: 1 = 暂停 (Cork), 0 = 恢复 (Uncork)
 * cb: 完成后的回调 (不需要传 NULL)
 */
pa_operation* pa_stream_cork(pa_stream *s, int b, pa_stream_success_cb_t cb, void *userdata);
```

暂停或恢复`pa_stream`对象的音频流。

```C
// 立即丢弃服务端缓冲区的所有数据。
// 用于：用户拖动进度条 (Seek)、切歌、停止播放。
pa_operation* pa_stream_flush(pa_stream *s, pa_stream_success_cb_t cb, void *userdata);
```

清空`pa_stream`对象的缓冲区，丢弃所有未处理的音频数据。

```C
// 等待缓冲区里的数据全部播放完毕。
// 用于：播放列表结束时，或者程序退出前，确保最后一句歌词唱完。
// 注意：只有当 buffer 空了，cb 回调才会被触发。
pa_operation* pa_stream_drain(pa_stream *s, pa_stream_success_cb_t cb, void *userdata);
```

等待`pa_stream`对象的缓冲区中的所有音频数据都被处理完毕。

```C
/*
 * attr: 新的缓冲区属性
 * 如果你只想改 tlength，其他不想改，就把其他的设为 (uint32_t)-1
 */
pa_operation* pa_stream_set_buffer_attr(pa_stream *s, 
                                        const pa_buffer_attr *attr, 
                                        pa_stream_success_cb_t cb, 
                                        void *userdata);
```

动态调整`pa_stream`对象的缓冲区属性。

```C
/*
 * rate: 新的采样率 (Hz)
 * 比如原流是 44100，你改成 48000，声音会变快且变尖（像花栗鼠）。
 * 改成 22050，声音会变慢且变粗。
 */
pa_operation* pa_stream_update_sample_rate(pa_stream *s, uint32_t rate, 
                                           pa_stream_success_cb_t cb, void *userdata);
```

动态重采样

```C
pa_operation* pa_stream_proplist_update(pa_stream *s, 
                                        PA_UPDATE_REPLACE, 
                                        pl, 
                                        NULL, NULL);
```

元数据更新

