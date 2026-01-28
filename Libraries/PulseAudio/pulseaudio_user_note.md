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

