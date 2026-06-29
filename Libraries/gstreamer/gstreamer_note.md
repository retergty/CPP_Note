# gstreamer

`GStreamer` 是一个流媒体框架，提供了一个管道（Pipeline）机制来处理多媒体数据。

参考文档

* [GStreamer Architecture](https://gstreamer.freedesktop.org/documentation/gstreamer/gstreamer-architecture.html)
* [官方的tutorials](https://gstreamer.freedesktop.org/documentation/tutorials/index.html)

## 架构

`GStreamer`的设计是极其经典的解耦架构，自上而下分为三层:

* `Application Layer`：应用层，负责与用户交互，控制管道的状态。过调用`GStreamer`的核心`API`来组装管道、控制播放状态（播放/暂停）、监听总线消息，以及处理音视频同步逻辑等。
* `Core Framework Layer`：核心框架层`libgstreamer`，提供了管道、元素、总线等基本组件的实现。只负责管理基础数据结构、状态机调度、时钟同步（Clock）以及插件的加载。不包含任何具体的编解码算法。
* `Plugin Layer`：插件层，包含了各种具体的元素（Element），如解码器、编码器、过滤器、混音器等。这些元素通过插件机制动态加载，可以由第三方开发者扩展。每个元素实现了特定的功能，如视频解码、音频处理等。

## 概念

### 元件（Element）

最基础的处理单元，分为`Source`(数据源，只出不进)、`Filter`(过滤器，有进有出，负责解码/转换等加工操作)、`Sink`(接收端，只进不出，负责渲染或输出)。

### 焊盘(Pad)

`Element`的输入/输出接口。数据从前一个``Element`的`Src Pad`流出，进入后一个`Element`的`Sink Pad`.

### 能力集(Caps)

附加在`Pad`上的“格式说明书”。它描述了流经该`Pad` 的数据格式（比如`audio/x-raw`,`format=S16LE`,`rate=44100`）。相邻的两个`Pad`必须通过“Caps 协商”达成一致，数据才能流通。

### 箱柜(Bin)

一个逻辑容器，内部可以包含多个`Element`。对于外部来说，一个`Bin`看起来就像是一个巨大的单一`Element`。`Bin`可以嵌套，形成层级结构。

### 管道(Pipeline)

`Pipeline`是一个特殊的`Bin`，它是整个媒体处理流程的顶层容器。所有的`Element`都必须放在一个`Pipeline`中才能工作。`Pipeline`负责管理数据流动、状态转换以及时钟同步,还负责提供全局时钟（Global Clock）和消息总线（Bus）。

## 数据结构

### GObject

`GObject` 是 `GStreamer` 中所有对象的基类，它主要提供了以下功能

* **引用计数模型**： 提供`g_object_ref`和`g_object_unref`。这就相当于`C++`中的`std::shared_ptr`控制块，用于追踪对象的生命周期，防止内存泄漏或野指针。
* **动态属性系统**： 提供了按字符串名字去读写对象属性的能力（`g_object_set/get`）。

### GstObject

`GStreamer`继承了`GObject`并派生出`GstObject`,它主要提供了以下功能

* **对象锁 (LOCK)**： 内部直接封装了互斥锁(类似`std::mutex`). 在多线程并发修改元件属性或改变管道拓扑时，保证状态的安全。
* **命名与层级**： 赋予了对象一个字符串名字（Name），并且引入了父子指针关系（Parent/Child），为构建拓扑树打下基础。

### GstMiniObject

`GstMiniObject`是一个轻量级的对象基类，主要用于表示数据流中的媒体数据比如GstBuffer（如音频帧、视频帧）以及控制事件（如Seek、EOS）。它主要提供了以下功能

* **引用计数模型**： 与`GObject`类似，提供了`gst_mini_object_ref`和`gst_mini_object_unref`，用于管理数据对象的生命周期。
* **内存管理**： 由于`GstMiniObject`通常用于高频率的数据流中，它的内存管理机制被优化为更高效的分配和释放，适合大量短生命周期对象的使用场景。

### GstElement

`GstElement`是`GstObject`的子类，代表一个具体的处理单元。它主要提供了以下功能

* **状态机 (State Machine)**： 拥有四个严格的状态：`NULL`（空闲）、`READY`（资源就绪）、`PAUSED`（暂停，数据预跑）、`PLAYING`（播放中）
* **pad管理 (Pad Management)**： 它可以拥有多个输入（Sink Pad）和输出（Source Pad）端口，允许它与其他`GstElement`连接。

### GstBin

`GstBin`是`GstElement`的子类，代表一个容器，可以包含多个`GstElement`。它主要提供了以下功能

* **打包封装**： 把一堆内部互相连接的`GstElement`进行封装，对外只暴露出几个pad。外界把它当成一个普通的单一元件来用。
* **状态转发**： 当把一个`Bin`设置为`PLAYING`时，它负责遍历其内的所有子元件，把它们也设置为`PLAYING`。

### GstPipeline

`GstPipeline`是`GstBin`的子类，代表整个媒体处理流程的顶层容器。它主要提供了以下功能

* **全局时钟 (Global Clock)**： 管道提供一个全局时钟，所有的`Element`都以这个时钟为基准进行时间戳的计算和同步。
* **消息总线 (Bus)**： 维护一个总线队列，收集管道内所有底层元件发出的消息（如错误、播放完毕），统一向上层应用层汇报。

### GstPad

`GstPad`是`GstElement`的输入/输出接口，代表数据流动的通道。它主要提供了以下功能

* **能力集 (Caps)**： 每个`Pad`都附带一个能力集（Caps），描述了流经该`Pad`的数据格式（如视频分辨率、音频采样率）。相邻的两个`Pad`必须通过“Caps 协商”达成一致，数据才能流通。
* **数据流动 (Data Flow)**： 数据从前一个`Element`的`Src Pad`流出，进入后一个`Element`的`Sink Pad`。`Pad`负责管理数据的流动和格式转换，确保不同`Element`之间的数据兼容。

### GstCaps

`GstCaps`是一个描述数据格式的结构，附加在`Pad`上。它主要提供了以下功能

* **格式描述**： 描述了流经`Pad`的数据格式，如`audio/x-raw,format=S16LE,rate=44100`。这就相当于一个“格式说明书”，告诉下一个`Element`它能接受什么样的数据。
* **Caps 协商**： 两个相邻的`Pad`通过协商来确定数据传输的格式，确保数据能够正确流动。

### GstBuffer

`GstBuffer（数据缓冲）`是`GStreamer`中最核心的数据结构，代表一块媒体数据（如音频帧或视频帧）。它包含内存指针和时间戳（PTS/DTS）。

**方向**：顺流而下（Downstream）。

严格遵循引用计数和写时拷贝（COW）机制，防止内存踩踏。

### GstMemory

`GstMemory`是数据实际保存的内存数据结构，一个`GstBuffer`中可能有多个`GstMemory`.

### GstEvent

`GstEvent（事件）`承载控制信号。比如 Seek（跳转播放位置）、Flush（清空缓存）、EOS（流结束）。

**方向**：既可以顺流而下，也可以逆流而上（Upstream）。

* **下行事件**： 比如`EOS` (`End Of Stream`，播放结束标记)，顺着数据流传遍所有元件。
* **上行事件**： 比如`SEEK` (拖动进度条)，从最末端的渲染器逆流而上，一直传给最源头的文件读取器，让它改变读取位置。

### GstMessage

`GstMessage（消息）`是从`Element`到应用层的通信机制。比如错误消息、状态变化、EOS通知等。

**方向**：从流水线抛向应用程序的主线程（Application Thread）。

`GstMessage`通过管道的消息总线（Bus）传递，应用层可以监听总线来获取这些消息并做出响应。

### GstClock

`GstClock`是`GStreamer`中的时钟系统，用于同步和管理时间戳（PTS/DTS）。它主要提供了以下功能

* **时间戳管理**： 管理数据流中的时间戳，确保音视频同步播放。
* **时钟同步**： 同步多个`GstClock`，确保时间戳的一致性。
* **时钟控制**： 控制时钟的运行和停止。

### GstSegment

`GstSegment`是`GStreamer`中的时间轴段，是`GStreamer`用来描述当前播放/处理的是媒体时间线中的哪一段，以及如何把`buffer`时间戳映射到`running-time`的结构体。

`GstSegment`通常来自`SEGMENT event`.

`GstSegment`里常见字段为：

* `format`：时间单位，比如`GST_FORMAT_TIME`
* `start`：`segment`开始位置
* `stop`：`segment`结束位置，可选
* `time`：映射到`running-time`的基准
* `rate`：播放速率，比如`1.0、2.0、-1.0`
* `base`：前面`segment`已累计的`running-time`偏移

## 状态机

`gstreamer`通过状态机来管理`Element`的生命周期。每个`Element`都有以下五种状态：

* `GST_STATE_NULL`：未初始化状态，资源未分配。
  * **应用层视角**：元件已被实例化，但尚未接入任何业务逻辑，或业务已彻底终止。
  * **插件层动作**：元件仅在内存中维持 C 语言的 GObject 结构体实例。绝对不占用任何系统级物理资源。设备句柄（如 /dev/video0、网络套接字、文件指针）必须处于关闭状态。所有分配的动态内存缓冲区均须在此阶段释放完毕。
* `GST_STATE_READY`：已准备好状态，资源已分配但未准备好处理数据。
  * **应用层视角**：流水线的拓扑结构已搭建完毕，要求系统为其保留必要的硬件或软件资源（如摄像头、麦克风、文件句柄等），但尚未进入数据处理阶段。
  * **插件层动作**：元件已分配必要的资源，执行基础的初始化操作，打开物理设备或底层 API 接口（如执行 open() 系统调用占用声卡或摄像头）；分配那些不依赖于具体媒体流格式（Caps）的全局资源。若此时设备被其他进程占用，状态切换将在此处立刻返回失败（FAILURE）。
* `GST_STATE_PAUSED`：暂停状态，准备好处理数据但不流动。
  * **应用层视角**：媒体流已准备就绪且停留在第一帧（或当前帧），可实现“零延迟”的瞬间播放。应用层通常在正式播放前或用户点击“暂停”时进入此状态。在此状态下，全局时钟（Global Clock）处于停止运行状态。
  * **插件层动作**：启动后台处理线程（Streaming Threads），开启数据流动。数据从`Source`元素开始流动，经过`Filter`元素的处理，最终到达`Sink`元素，但全局时钟（Global Clock）保持停止状态。此时，数据流动但不进行时间戳的更新，所有帧的时间戳（PTS/DTS）保持不变。
* `GST_STATE_PLAYING`：播放状态，数据流动中。
  * **应用层视角**：音视频数据正在按照预定的时间戳进行实时处理和渲染,下达此指令后，应用层交出控制权，数据流转完全交由底层框架接管，直至触发 EOS（流结束）或外部干预。
  * **插件层动作**：全局时钟（Global Clock）开始运行，数据流动并且时间戳（PTS/DTS）根据时钟进行更新。`Sink`元素根据时间戳进行同步渲染，确保音视频同步播放。

不只是`Element`，`bin`,`Pipeline`也有一个状态机，状态转换会递归地影响所有子元素。

## 常见插件

### 常见Bin

这些`Bin`是由多个`Element`组合而成的常用功能模块，封装了复杂的处理流程，对外提供简单的接口。

#### playbin

它管理媒体播放的各个方面，从源到显示，包括解复用和解码。

#### uridecodebin

它负责从URI（如文件路径或网络地址）自动识别媒体类型，选择合适的解复用器和解码器进行处理。

它内部逻辑大致为

1. 自动协议适配 (The URI part)，根据URI的前缀（如`file://`、`http://`）选择合适的`Source`元素来读取数据。
2. 自动格式嗅探与解码 (The Decode part)，根据数据流的前几个字节（Magic Number）自动识别媒体格式，选择合适的`Demuxer`（解复用器）和`Decoder`（解码器）进行处理。
3. 动态端口 (Dynamic Source Pads)，`uridecodebin`会根据实际解析到的媒体流动态创建输出端口（Source Pad），并通过信号通知应用层进行连接。

#### decodebin

它负责自动识别输入数据的格式，并选择合适的解码器进行处理。与`uridecodebin`不同的是，`decodebin`不关心数据的来源（URI），它只负责解码功能。

`uridecodebin`内部会创建一个`decodebin`来处理解码任务，`decodebin`会根据输入数据的格式自动选择合适的解码器进行处理，并动态创建输出端口（Source Pad）供后续元素连接。

### 文件输入输出

#### filesrc

这个元件负责从文件系统中读取数据。它是一个`Source`元素，只有输出端口（Source Pad）。它支持随机访问，可以通过设置属性来指定读取的文件路径。

此元素读取本地文件并生成`ANY Caps`的媒体。如果想获取媒体的正确`Caps`，可以在后面接上`typefind`元件，或将`filesrc`的 `typefind`属性设置为`TRUE`。

#### filesink

此元件会将接收到的所有媒体写入文件。使用`location`属性指定文件名。

### 网络输入输出

#### souphttpsrc

这个元件负责从HTTP服务器上读取数据。它是一个`Source`元素，只有输出端口（Source Pad）。它支持HTTP协议，可以通过设置属性来指定URL。

### 测试流

#### videotestsrc

这个元件会生成一个测试视频流，常用于调试和测试。它是一个`Source`元素，只有输出端口（Source Pad）。可以通过设置属性来指定生成的视频模式（如颜色条、雪花等）。

#### audiotestsrc

这个元件会生成一个测试音频流，常用于调试和测试。它是一个`Source`元素，只有输出端口（Source Pad）。可以通过设置属性来指定生成的音频模式（如正弦波、方波等）。

### 视频适配转换

#### videoconvert

这个元件负责进行视频格式转换。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以处理不同的视频格式之间的转换，如颜色空间转换、像素格式转换等。

它是使用CPU进行软件转换的，如果需要使用GPU加速，可以使用`nvvidconv`（NVIDIA）或`vaapipostproc`（Intel）等专用的硬件加速元素。

#### videorate

这个元件负责进行视频帧率转换。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以调整视频的帧率，通过插入或丢弃帧来实现。

它的校正方法是通过丢弃和复制帧来实现的，没有使用复杂的算法来插值帧。

#### videoscale

这个元件负责进行视频缩放。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以调整视频的分辨率，通过插值算法来实现缩放。

它的缩放算法是通过插值实现的，对CPU性能有一定的要求，如果需要使用GPU加速，可以使用`nvvideoscale`（NVIDIA）或`vaapipostproc`（Intel）等专用的硬件加速元素。

### 音频适配转换

#### audioconvert

这个元件负责进行音频格式转换。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以处理不同的音频格式之间的转换。

它负责进行例如数据类型与位深转换，声道混合与重映射，字节序与交织转换。

#### audioresample

这个元件负责进行音频采样率转换。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以调整音频的采样率，通过插值算法来实现。

它的采样率转换算法是通过插值实现的，对CPU性能有一定的要求，如果需要使用GPU加速，可以使用`nvaudiosample`（NVIDIA）或`vaapipostproc`（Intel）等专用的硬件加速元素。

```bash
gst-launch-1.0 uridecodebin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! audioresample ! audio/x-raw,rate=4000 ! audioconvert ! autoaudiosink
```

#### audiorate

该元件接收带有时间戳的原始音频帧流，并根据需要插入或删除样本来生成完美的音频流。

它根据pts和duration计算下一时刻的时间点，如果当前帧的时间戳与预期时间点不匹配，则会插入或删除样本来调整时间戳，使得输出音频流的时间戳连续且符合预期。

### 多线程

#### queue

这个元件负责：

* 数据会被放入队列，直到达到设定的上限。此时，生产者线程会被阻塞，直到消费者线程从队列中取出数据并释放空间。
* 该队列会在源`Pad`上创建一个新线程，以解耦目标`Pad`和源`Pad`上的处理过程。

此外，队列在即将变为空或满时（根据一些可配置的阈值）会触发信号，并且可以指示队列在满时丢弃缓冲区而不是阻塞。

#### queue2

这个元件不是`queue`的改进版本，而是一个完全独立的实现。它提供了更高效的内存管理和更灵活的配置选项。

它实现了`queue`的所有功能，并且增加了以下功能：

* 它可以将缓冲区存储在磁盘上，而不是内存中，以处理更大的数据量。它还用更通用、更方便的缓冲消息替换了信号。

通常`queue2`用于网络缓冲。

#### multiqueue

该元素为多个数据流同时提供队列，并通过以下方式简化了管理：允许某些队列在其他数据流未接收数据时增长，或者允许某些队列在未连接到任何数据流时丢弃数据（而不是像更简单的队列那样返回错误）。此外，它还同步不同的数据流，确保任何一个数据流都不会领先于其他数据流太多。

这是一个高级元素。它位于 decodebin 内部，但在普通的播放应用程序中，很少需要自己实例化它。

#### tee

这个元件负责将数据流分成多个分支。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它可以将输入的数据流复制到多个输出端口，允许数据流被多个后续元素同时处理。

最好在每个分支上使用`queue`解耦，为每个分支提供独立的线程。否则，一个分支的数据流阻塞会导致其他分支也阻塞。

`tee`元件不会实际发生数据复制，而是通过引用计数机制共享数据缓冲区（`GstBuffer`）。当数据流通过`tee`时，`GstBuffer`的引用计数会增加，每个分支都持有对同一缓冲区的引用。当所有分支处理完该缓冲区后，引用计数会减少到零，缓冲区才会被真正释放。这种机制避免了不必要的数据复制，提高了性能。

### 能力Caps

#### capsfilter

这个元件本身并不修改数据，而是对数据格式施加限制。如果上游元素输出的数据格式不符合`capsfilter`设置的能力（Caps），会在协商阶段返回`not-negotiated`.

```bash
gst-launch-1.0 videotestsrc ! video/x-raw, format=GRAY8 ! videoconvert ! autovideosink
```

#### typefind

这个元件用来确定流中包含的媒体类型。它按优先级顺序应用类型查找函数。检测到类型后，它会将源`Pad Caps`设置为找到的媒体类型，并发出`have-type`信号。

### 调试使用

#### fakesink

这个元件会丢弃所有接收到的数据，常用于调试和测试。它是一个`Sink`元素，只有输入端口（Sink Pad）。在调试时，它可以替换原有的sink元素，排除它们的干扰。

#### identify

这个元件会分析输入数据并输出相关信息，常用于调试和测试。它是一个`Filter`元素，既有输入端口（Sink Pad）也有输出端口（Source Pad）。它具有一些有用的诊断功能，例如偏移量和时间戳检查，以及缓冲区丢弃来模拟丢包。

### 平台相关

平台相关的插件通常是针对特定硬件或操作系统优化的元素，利用平台特有的功能来提高性能或提供额外的功能。

#### 通用平台

##### glimagesink

这个元件基于`OpenGL`或`OpenGL ES`进行视频渲染，适用于支持`OpenGL`的系统。它是一个`Sink`元素，只有输入端口（Sink Pad）。它利用GPU加速进行视频渲染，提供更高效的性能和更好的视觉效果。它可以分解为`glupload ! glcolorconvert ! glimagesinkelement`，以便在管道中插入进一步的`OpenGL`硬件加速处理。

这个元件通过`OpenGL`利用了`GPU`可编程的能力，可以自定义渲染效果，例如添加滤镜、调整颜色等。它还支持与其他`OpenGL`元素（如`glfilter`）的无缝集成，允许在渲染前对视频帧进行复杂的处理。但是会增加GPU的负载，可能会导致性能下降，尤其是在资源有限的设备上。

#### Linux平台

##### kmssink

这个元件使用`Direct Rendering Manager (DRM)`和`Kernel Mode Setting (KMS)`接口进行视频渲染，适用于Linux系统。它是一个`Sink`元素，只有输入端口（Sink Pad）。它直接与内核交互进行视频渲染，提供低延迟和高性能的输出。

它有以下的特性：

* **物理直通**:`kmssink`直接越过了所有的桌面系统、窗口管理器，直接敲开了`Linux`内核的大门，对着底层的显卡驱动（`VOP2`）进行渲染。
* **抢占屏幕**： 它会直接霸占一个物理屏幕图层（`Hardware Plane`）。

##### waylandsink

这个元件使用`Wayland`协议进行视频渲染，适用于支持`Wayland`的Linux系统。它是一个`Sink`元素，只有输入端口（Sink Pad）。它通过`Wayland`协议与显示服务器通信进行视频渲染，提供现代化的显示支持和更好的性能。

* **申请许可**： `waylandsink`会向`Wayland`显示服务器申请一个窗口（`Surface`），并在该窗口上进行视频渲染。
* **受限渲染**： 由于`Wayland`的安全模型，`waylandsink`只能在申请到的窗口上进行渲染，无法直接访问物理屏幕。这意味着它无法实现真正的全屏渲染，必须依赖于显示服务器的窗口管理功能来实现全屏效果。
* **零拷贝传递**： `waylandsink`支持通过`Wayland`的`linux-dmabuf`协议机制实现零拷贝的视频帧传递，减少CPU负载，提高性能。

它的优点是

* **完美的`UI`融合**：天生支持窗口化。适合配合`Qt Wayland`或者`GTK`开发带有复杂按钮、侧边栏、悬浮窗的现代图形界面应用。
* **多任务友好**：允许多个视频播放器、多个程序和平共处。

它的劣势是

* **系统依赖重**： 必须启动一个`Wayland`桌面环境(比如`Weston`).这会占用一定的系统内存（通常几十兆）.
* **微小延迟**： 毕竟中间多了一层`Wayland`混成器的调度（IPC 通信），理论上比`kmssink`多出一点调度延迟（通常小于一帧，肉眼不可见）。

##### alsasink

这个元件使用`ALSA`（Advanced Linux Sound Architecture）进行音频输出，适用于Linux系统。它是一个`Sink`元素，只有输入端口（Sink Pad）。它直接与`ALSA`驱动交互进行音频输出，提供低延迟和高性能的音频播放。

##### pulsesink

这个元件使用`PulseAudio`进行音频输出，适用于Linux系统。它是一个`Sink`元素，只有输入端口（Sink Pad）。它通过`PulseAudio`服务器进行音频输出，提供更好的兼容性和更多的功能选项。
