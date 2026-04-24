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

### GstEvent

`GstEvent（事件）`承载控制信号。比如 Seek（跳转播放位置）、Flush（清空缓存）、EOS（流结束）。

**方向**：既可以顺流而下，也可以逆流而上（Upstream）。

* **下行事件**： 比如`EOS` (`End Of Stream`，播放结束标记)，顺着数据流传遍所有元件。
* **上行事件**： 比如`SEEK` (拖动进度条)，从最末端的渲染器逆流而上，一直传给最源头的文件读取器，让它改变读取位置。

### GstMessage

`GstMessage（消息）`是从`Element`到应用层的通信机制。比如错误消息、状态变化、EOS通知等。

**方向**：从流水线抛向应用程序的主线程（Application Thread）。

`GstMessage`通过管道的消息总线（Bus）传递，应用层可以监听总线来获取这些消息并做出响应。

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
