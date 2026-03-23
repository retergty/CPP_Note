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

### GstBuffer

`GstBuffer（数据缓冲）`是`GStreamer`中最核心的数据结构，代表一块媒体数据（如音频帧或视频帧）。它包含内存指针和时间戳（PTS/DTS）。

**方向**：顺流而下（Downstream）。

严格遵循引用计数和写时拷贝（COW）机制，防止内存踩踏。

### GstEvent

`GstEvent（事件）`承载控制信号。比如 Seek（跳转播放位置）、Flush（清空缓存）、EOS（流结束）。

**方向**：既可以顺流而下，也可以逆流而上（Upstream）。

### GstMessage

`GstMessage（消息）`是从`Element`到应用层的通信机制。比如错误消息、状态变化、EOS通知等。

**方向**：从流水线抛向应用程序的主线程（Application Thread）。

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
