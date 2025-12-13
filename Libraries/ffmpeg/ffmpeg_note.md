# ffmpeg

## 概念

### 层级

FFmpeg分为几个层级：

* 封装层(Format Layer):负责文件格式的读写，比如`mp4`,`flv`,`mkv`等，进行`Packet`的封装和解封装，库文件是`libavformat`，核心结构体是`AVFormatContext`
* 编解码层(Code/Decode Layer):负责音视频编解码，包`Packet`和数据`frame`的转换，比如`H.264`,`AAC`等，库文件是`libavcodec`，核心结构体是`AVCodecContext`
* 媒体处理层(Media Layer):负责对音视频数据进行处理，比如像素格式转换，缩放，采样率转换等，库文件是`libswscale`(视频)，`libswresample`(音频)
* 设备层(Device Layer):负责和硬件设备交互，比如摄像头，麦克风，显示器等，库文件是`libavdevice`

### 文件内存格式

视频包(V)和音频包(A)等按照时间顺序在内存交错存放.例如`[Header] [V_Packet_1] [A_Packet_1] [A_Packet_2] [V_Packet_2] [V_Packet_3] ...`

#### FLV结构

`FLV`文件由多个如下的结构构成，他们链式存储

```txt
[ Tag Header (11字节) ] + [ Data (H.264/AAC数据) ] + [ PreviousTagSize (4字节) ]
```

* Tag Header:包含
    * 类型：是视频(0x09) 还是 音频(0x08)
    * Data Size： Data段大小
    * Timestamp：时间戳

#### mp4结构

`mp4`结构把数据和索引分开放

* 数据放在 mdat (Media Data)，是纯粹的`H.264/AAC`数据，没有分割符，也没有时间戳
* 索引放在 moov (Movie) 箱子里

### 解封装demux

文件如`.mp4`，将里面的视频，音频，字幕解开来

### 压缩包AVPacket

解封装器将文件解封装后，得到了压缩数据包.

假如是 H.264 编码，这里面就是一串 0101 的二进制码流（NALU），非常小（比如几十 KB）。

包含 DTS/PTS（解码时间戳/显示时间戳），决定了这帧画面什么时候播。

### 原始帧AVFrame

解码器将压缩包“解压”后，还原成的原始图像数据.

* 像素数据（Pixels）。通常是`YUV`格式（Y=亮度，U/V=色度）。

## 常用结构体

### AVFormatContext

`AVFormatContext`是重要的结构体，管理了几乎所有的内容，代表一个打开的文件或是流。

#### 常见成员变量

* `const AVInputFormat *iformat`
    * 比如打开开的是`MP4`，这里就指向`mov,mp4,m4a...`的解封装器
* `const AVOutputFormat *oformat`
    * 如果想存FLV，就会指向FLV的封装器
* `AVIOContext *pb`,底层的 I/O 上下文。它负责真正的读写操作（文件 fread, 网络 socket recv）

* `unsigned int nb_streams`,表示有多少条流
* `AVStream **streams`,指针数组，成员便是一个流对象.
    * 比如访问视频流参数`ctx->streams[video_index]->codecpar`

* `int64_t duration`文件总时长，单位是`AV_TIME_BASE`,微秒
* `int64_t bit_rate`总码率(bps)
* `AVDictionary *metadata`键值对信息
    * 比如`title="Avengers"`,`artist="Marvel"`,`rotate="90"`

* `AVIOInterruptCB interrupt_callback`中断回调函数，用于网络超时等情况.这个回调函数会在底层`I/O`操作时被周期性调用，如果返回非0值，则中断当前操作.
    
    ```CPP
    typedef struct AVIOInterruptCB {
        int (*callback)(void*); // 回调函数指针
        void *opaque;           // 用户数据指针 (传给回调函数的参数)
    } AVIOInterruptCB;
    ```

### AVStream

一个媒体文件通常包含多个流（视频流、音频流、字幕流），每一个流都在`AVFormatContext`中对应一个 `AVStream`对象。

`AVStream`是流的描述符，里面并没有实际的数据.

* 上级： `AVFormatContext` (包含了一个`streams`指针数组)。

* 下级： `AVCodecParameters` (包含了分辨率、编码格式等具体参数)。

```CPP
// 访问第 i 条流
AVStream* stream = format_ctx->streams[i];
```

#### 常见成员变量

* `AVCodecParameters *codecpar`描述了流里数据的参数，比如编码ID，分辨率等关键信息，传递给解码器来使用
* `AVRational time_base`时间单位，它是一个分数，比如`1/90000`是时间戳(PTS/DTS)的单位.

    $$
    实际时间(秒) = PTS \times av\_q2d(time\_base)
    $$

    如果`time_base`是`1/1000`(1ms),且一帧的PTS是`500`那么这帧的显示时间就是$500 \times 0.001=0.5 秒$
* `int index`在`AVFormatContext`中流的索引，用来一一对应流.
* `int64_t duration;`这个流的总时长（单位是 time_base）。
* `int64_t nb_frames;`这个流一共有多少帧，如果未知则为0.
* `AVRational avg_frame_rate;`平均帧率

### AVCodecParameters

`AVStream`里面的参数，传递给编解码层。存储了编解码器的参数

#### 常见成员变量

* `enum AVMediaType codec_type;`流的类型，比如`AVMEDIA_TYPE_VIDEO`视频流，`AVMEDIA_TYPE_AUDIO`音频流,`AVMEDIA_TYPE_DATA`雷达数据等
* `enum AVCodecID codec_id;`编码的算法，比如`AV_CODEC_ID_H264`,`AV_CODEC_ID_AAC`,`AV_CODEC_ID_MJPEG`.
* `int format;`像素格式,对于视频则是`AVPixelFormat`，对于音频则是`AVSampleFormat`,比如`AV_PIX_FMT_YUV420P`.
* `uint8_t *extradata;`额外数据,用于初始化解码器，这个是每个解码器独有的.

视频使用的

* `int width;int height;`分辨率，如`1920x1080`

音频

* `int sample_rate;`采样率（如 44100）
* `uint64_t channel_layout;`声道布局

### AVCodec

`AVCodec`存储编解码器算法

`AVCodec`是一个静态、只读的结构体，定义了特定的编解码器的所有固有属性和能力，用于之后创建`AVCodecContext`.

里面有许多函数指针，表示编解码器调用的函数.

#### 常见成员变量

* `name`,`long_name`,名字，一个是简写，一个是全称
* `int capabilities;`表示这个编解码器能力的掩码.
    * `AV_CODEC_CAP_DR1`支持 "Direct Rendering"（零拷贝解码，性能很高）。
    * `AV_CODEC_CAP_DELAY`表示有延迟的，给它`Packet`，可能不会立刻吐出`Frame`，最后需要 `Flush`
* `const enum AVPixelFormat *pix_fmts`; 支持的像素格式

#### 软解硬解

对于同一个编码标准`codec_id`,可能有多个不同的`AVCodec`实现，比如`AV_CODEC_ID_H264`

* `h264`默认软解
* `h264_nvenc`(NVIDIA 硬件编码)
* `h264_cuvid`(NVIDIA 硬件解码)

### AVCodecContext

`AVCodecContext`编解码器实例.维护解码器状态（上下文）.

和`AVCodecParameters`的区别是，它是动态的，在编解码过程会改变。而`AVCodecParameters`是静态的，从文件头里读取出来的。

#### 常见成员变量

* `int flags;int flags2;`,`AV_CODEC_FLAG_*,AV_CODEC_FLAG2_*`控制编解码器的flags.

性能控制

* `int thread_count;`线程数.默认是单线程
* `int thread_type;`多线程类型
    * `FF_THREAD_FRAME`同时编解码多个帧（延迟高，吞吐量大）
    * `FF_THREAD_SLICE`将一个帧分成多块，（延迟低，适合直播）

格式控制

这些变量都是由用户设定，但是也是会被编解码器修改.

* `int width, height;`图像宽高
* `enum AVPixelFormat pix_fmt;`输出的像素格式如`YUV420`
* `int gop_size;`:编码时使用的关键帧间隔.

硬件加速接口

* `AVBufferRef *hw_device_ctx;`硬件加速编解码实例

### AVPacket

`AVPacket`是压缩数据包，存储从封装层读取的压缩数据.

* 对于视频，就是一帧压缩的`H.264`数据
* 对于音频，就是一帧压缩的`AAC`数据

#### 常见成员变量

* `uint8_t *data;`指向压缩数据的指针
* `int size;`数据大小，单位字节
* `int64_t pts;`显示时间戳(单位是流的 time_base)，根据它来决定什么时候显示这帧画面
* `int64_t dts;`解码时间戳(单位是流的 time_base)，根据它来决定什么时候解码这帧画面
* `int stream_index;`这个包属于哪个流(对应 AVFormatContext->streams 数组的索引)
* `int flags;`标志位，比如`AV_PKT_FLAG_KEY`表示关键帧

#### 内存管理

`AVPacket`使用了引用计数的内存管理机制，避免频繁的内存拷贝和分配.

当复制`AVPacket`时，实际上只是增加了引用计数，而不是复制数据本身.

当不再需要`AVPacket`时，调用`av_packet_unref`释放它，减少引用计数，当引用计数为0时，才真正释放内存.

### AVFrame

`AVFrame`是解码后的原始数据帧，存储解码后的音视频数据.

* 对于视频，就是图像的像素数据，比如`YUV420P`
* 对于音频，就是 PCM 原始音频数据

#### 常见成员变量

* `uint8_t *data[AV_NUM_DATA_POINTERS];`数据指针数组，存储图像的各个平面数据指针.

    对于`YUV420P`格式：

    * `data[0]`指向 Y 平面数据
    * `data[1]`指向 U 平面数据
    * `data[2]`指向 V 平面数据

* `int linesize[AV_NUM_DATA_POINTERS];`每个平面的行大小(单位字节)，用于计算每一行数据的起始位置.为了内存对齐，行大小可能大于图像宽度.大于宽度的部分是填充字节.
* `int width, height;`图像宽高
* `enum AVPixelFormat format;`像素格式，比如`AV_PIX_FMT_YUV420P`
* `int64_t pts;`显示时间戳(单位是流的 time_base)

#### 内存管理

`AVFrame`也使用了引用计数的内存管理机制，避免频繁的内存拷贝和分配.

当复制`AVFrame`时，实际上只是增加了引用计数，而不是复制数据本身.

当不再需要`AVFrame`时，调用`av_frame_unref`释放它，减少引用计数，当引用计数为0时，才真正释放内存.

## 常用函数

### `avformat_open_input`

```CPP
int avformat_open_input(AVFormatContext **ps, const char *url, ff_const59 AVInputFormat *fmt, AVDictionary **options);
```

打开文件或者是流并解析相关信息读取文件头，获取格式信息，是解封装第一个调用的函数。

* `AVFormatContext **ps`,`AVFormatContext`对象
* `url`地址,可以是文件路径，也可以是网络流
* `fmt`强制指定输入格式
* `options`字典，用于配置底层的参数

### `avformat_find_stream_info`

```CPP
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);
```

阻塞并读取Packet来猜测格式信息，在`avformat_open_input`后调用，对于有完整文件头的文件没用，但是对于没有完整文件头的流来说，它会猜测内容。

调用结束后，`AVFormatContext`中就会有关于流的信息如

* `nb_streams: 2` (发现了一个视频流，一个音频流)。
* `streams[0]->time_base: 1/90000` (时间基准)。
* `streams[0]->codecpar->codec_id`: AV_CODEC_ID_H264。
* `streams[0]->codecpar->width: 1920`。
* `streams[0]->codecpar->extradata`: SPS/PPS 数据 (这是解码的关键！)。

### avcodec_find_decoder

```CPP
AVCodec *avcodec_find_decoder(enum AVCodecID id);
```

根据解码器ID查找并分配内存给解码器.

输入参数

* `enum AVCodecID id`,解码器ID，通常就是之前获取的`AVCodecParameters->codec_id`.

输出参数

* `AVCodec*`指针，获取的解码器.

#### 使用硬件解码器

默认`avcodec_find_decoder`返回的是软件解码器，如果需要使用硬件解码器，应该使用`avcodec_find_decoder_by_name`手动指定名字

```CPP
AVCodec *avcodec_find_decoder_by_name(const char *name);
```

```CPP
// 显式指定找 NVIDIA 的硬件解码器
const AVCodec* codec = avcodec_find_decoder_by_name("h264_cuvid"); 
// 或者 "h264_nvenc" (编码), "h264_qsv" (Intel 核显)

if (!codec) {
    std::cout << "没找到硬解驱动，回退到软解..." << std::endl;
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
}
```

### avcodec_alloc_context3

```CPP
AVCodecContext *avcodec_alloc_context3(const AVCodec *codec);
```

类似于构造函数，接受`AVCodec`,构建`AVCodecContext`，此时`AVCodecContext`是空的，需要手动填充参数.

### avcodec_parameters_to_context

```CPP
int avcodec_parameters_to_context(AVCodecContext *codec_ctx, const AVCodecParameters *codecpar);
```

将`AVCodecParameters`的参数拷贝到`AVCodecContext`中.

拷贝后，`AVCodecContext`就有了分辨率、编码格式、Extradata等参数，可以直接用来打开解码器.

### avcodec_open2

```CPP
int avcodec_open2(AVCodecContext *codec_ctx, const AVCodec *codec, AVDictionary **options);
```

打开编解码器，必须在`AVCodecContext`参数填充完整后调用.

参数

* `AVCodecContext *codec_ctx`,编解码器上下文，启动成功后，它的状态变成`open`
* `const AVCodec *codec`,编解码器对象
* `AVDictionary **options`,可选参数字典

进行三步操作：

* 参数合法性检查，验证`AVCodecContext`参数是否合法。
* 初始化解码器内部状态，分配内存等。
* 启动解码器线程（如果多线程被启用）。

### av_packet_alloc,av_frame_alloc

```CPP
AVPacket *av_packet_alloc(void);
AVFrame *av_frame_alloc(void);
```

分配并使用默认值初始化`AVPacket`和`AVFrame`结构体.它仅仅分配内存，并不会分配数据缓冲区`data`.

### av_read_frame

```CPP
int av_read_frame(AVFormatContext *fmt_ctx, AVPacket *pkt);
```

从`AVFormatContext`读取下一个`AVPacket`.

输入

* `AVFormatContext *fmt_ctx`,解封装上下文

输出

* `AVPacket *pkt`,读取到的压缩包，调用前必须分配内存.

返回值

* `0`成功
* 负值失败或者文件结束

进行如下操作：

* 从底层 I/O 读取数据（文件 fread, 网络 socket recv）。
* 分配`AVPacket`的数据缓冲区（`data`）。
* 解析封装格式，提取出一个完整的`AVPacket`。
* 填充`AVPacket`的各个字段（data, size, pts, dts, stream_index 等）。
* 更新`AVFormatContext`的内部状态（如文件位置等）。
* 返回读取结果。

#### 注意事项

* `av_read_frame`返回的数据包是时间序交错的，可能不是单一流的连续包.需要根据`pkt->stream_index`来区分是视频包还是音频包.

    比如

    1. 调用第 1 次 -> 返回`AVPacket`(Stream #0, Video, DTS=0)
    2. 调用第 2 次 -> 返回`AVPacket`(Stream #1, Audio, DTS=0)
    3. 调用第 3 次 -> 返回`AVPacket`(Stream #1, Audio, DTS=23)
    4. 调用第 4 次 -> 返回`AVPacket`(Stream #0, Video, DTS=33)

    ```CPP
    if (pkt->stream_index == video_idx) {
        // 喂给视频解码器
    } else if (pkt->stream_index == audio_idx) {
        // 喂给音频解码器
    } else {
        // 字幕或其他数据，忽略
    }
    ```

* `av_read_frame`分配的`AVPacket->data`内存需要手动释放，调用`av_packet_unref(pkt)`释放.
* `av_read_frame`是阻塞调用，如果是网络流，可能会因为网络延迟而阻塞较长时间，可以设置超时选项.或设置超时回调函数.

### avcodec_send_packet

```CPP
int avcodec_send_packet(AVCodecContext *codec_ctx, const AVPacket *pkt);
```

将`AVPacket`送入解码器，将`AVPacket`放入解码队列，等待解码器线程处理.

输入参数

* `AVCodecContext *codec_ctx`,解码器上下文
* `const AVPacket *pkt`,要解码的压缩包，可以是`nullptr`，表示刷新解码器.

返回值

* `0`成功，Packet 已经送入解码器队列
* `AVERROR(EAGAIN)`解码器内部的输入缓冲区已经满了，或者内部积压了太多的`Frame`等待输出。此时解码器拒绝接收新的 Packet。
    * 解决方法：调用`avcodec_receive_frame`获取更多的`Frame`，直到返回非`EAGAIN`为止，然后再继续调用`avcodec_send_packet`送入新的 Packet。
* `AVERROR_EOF`解码器已经被刷新，不能再送入新的 Packet。此时解码器进入了Drain状态。任何非空的 Packet 都会被拒绝。
    * 解决方法：不要再调用`avcodec_send_packet`，直接调用`avcodec_receive_frame`获取剩余的`Frame`，直到返回`AVERROR_EOF`为止。然后重新初始化解码器。
* `AVERROR(EINVAL)`解码器没有被正确打开，或者参数无效.
* `AVERROR(ENOMEM)`内存不足，无法分配`AVFrame`的缓冲区.

### avcodec_receive_frame

```CPP
int avcodec_receive_frame(AVCodecContext *codec_ctx, AVFrame *frame);
```

从解码器获取解码后的`AVFrame`

输入参数

* `AVCodecContext *codec_ctx`,解码器上下文
* `AVFrame *frame`,由调用者分配的一个空`AVFrame`结构体（通常用`av_frame_alloc`分配）。如果`frame`里原本有数据，FFmpeg 会先自动调用`av_frame_unref`把它清空，然后再填充新的数据.

返回值

* `0`成功，`frame`里有数据
* `AVERROR(EAGAIN)`解码器内部没有足够的数据来输出一个完整的`Frame`。通常是因为还没有送入足够的`Packet`。
    * 解决方法：调用`avcodec_send_packet`送入更多的`Packet`，然后再调用`avcodec_receive_frame`尝试获取`Frame`。
* `AVERROR_EOF`解码器已经被刷新，所有的`Frame`都已经输出完毕，没有更多的`Frame`可以获取。
    * 解决方法：不要再调用`avcodec_receive_frame`，解码过程已经结束。如果需要重新解码新的数据，需要重新初始化解码器。

### av_frame_unref，av_packet_unref

```CPP
void av_frame_unref(AVFrame *frame);
void av_packet_unref(AVPacket *pkt);
```

释放`AVFrame`或`AVPacket`的内部数据缓冲区，减少引用计数，当引用计数为0时，才真正释放内存.

在使用完`AVFrame`或`AVPacket`后，必须调用这个函数释放它们占用的内存.

### av_packet_free，av_frame_free

```CPP
void av_packet_free(AVPacket **pkt);
void av_frame_free(AVFrame **frame);    
```

释放`AVPacket`或`AVFrame`结构体本身的内存，同时也会调用`av_packet_unref`或`av_frame_unref`释放内部数据缓冲区.

## 常用场景

### 解封装

```CPP
// 1. 创建并打开：FFmpeg 自动分配内存并解析文件头填充字段
AVFormatContext* fmt_ctx = nullptr;
avformat_open_input(&fmt_ctx, "video.mp4", nullptr, nullptr);

// 2. 补全流信息：有些格式头部没信息，需要读几帧分析
avformat_find_stream_info(fmt_ctx, nullptr);

// 3. 使用：读取 Packet
av_read_frame(fmt_ctx, packet); // 从 fmt_ctx->pb 读数据

// 4. 销毁：必须用这个专门的函数，因为它会释放内部的 streams 和 priv_data
avformat_close_input(&fmt_ctx);
```

### 初始化解码器

```CPP
// 1. 从流中获取参数指针 (只是个指针，指向 format_ctx 里的内存)
AVCodecParameters* codec_params = format_ctx->streams[video_index]->codecpar;

// 2. 找解码器 (根据 ID)
const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);

// 3. 分配解码器上下文 (这是个空壳)
AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);

// 【改装】手动调整参数 (可选)
codec_ctx->thread_count = 4; // 开启多线程
codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY; // 告诉解码器我要低延迟

// 4. 【核心一步】将参数从 Parameters 拷贝到 Context
// 这一步把“清单”上的分辨率、编码格式、Extradata 全部填进了“解码器”里
if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
    // 报错处理
}

// 5. 正式打开解码器
avcodec_open2(codec_ctx, codec, nullptr);
```

### 解码数据包

```CPP
// --- 4. 准备 Packet 和 Frame ---
AVPacket *packet = av_packet_alloc(); // 存放从文件读出的压缩数据
AVFrame *frame = av_frame_alloc();    // 存放解码后的原始图像

int frame_count = 0;

// --- 5. 读取循环 (Demux Loop) ---
// av_read_frame 返回 >= 0 表示读取成功
while (av_read_frame(format_ctx, packet) >= 0)
{
    // 我们只处理视频流，忽略音频流
    if (packet->stream_index == video_stream_index)
    {

        // --- 6. 解码 (Decode: Send/Receive Model) ---

        // A. 发送 Packet 给解码器
        int ret = avcodec_send_packet(codec_ctx, packet);
        if (ret < 0)
        {
            std::cerr << "Error sending packet for decoding" << std::endl;
            break;
        }

        // B. 从解码器接收 Frame (可能会有多个，或者没有)
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                // EAGAIN: 需要更多 Packet 才能输出 Frame (正常现象)
                // EOF: 文件结束
                break;
            }
            else if (ret < 0)
            {
                std::cerr << "Error during decoding" << std::endl;
                break;
            }

            // --- 7. 获取到了原始图像 (AVFrame) ---
            std::cout << "Decoded frame " << codec_ctx->frame_number
                        << " (" << frame->width << "x" << frame->height << ")"
                        << " format: " << frame->format << std::endl;

            // 处理 frame (渲染、保存等)
            frame_count++;
            // 每次用完 Frame 后，虽然 receive_frame 会自动 reset，但显式清理是个好习惯
            av_frame_unref(frame);
        }
    }

    // 每次用完 Packet，必须重置引用计数，否则内存泄漏
    av_packet_unref(packet);
}
```