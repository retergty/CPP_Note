# GStreamer命令行工具

## gst-inspect-1.0

* 查看所有可用插件：

  ```bash
  gst-inspect-1.0
  ```

* 搜索特定插件（比如查找所有的 H.264 相关插件）：

  ```bash
  gst-inspect-1.0 | grep h264
  ```

* 查看某个特定元件的详细参数：

  ```bash
  gst-inspect-1.0 v4l2src
  ```

  * Pad Templates (衬垫模板): 描述了该元素的输入/输出接口（Pad）的类型和能力（Caps）。比如 `v4l2src` 的 `Src Pad` 可能支持 `video/x-raw` 格式。
  * Element Properties (元件属性):可以在命令行里给它传什么参数（比如 device=/dev/video0 指定设备节点）。

## gst-launch-1.0

这是`GStreamer`最核心的命令行工具。它允许用户通过一套特定的语法规则，把多个`Element`串联成一条完整的`Pipeline`并立刻执行。

### 基本语法

```bash
gst-launch-1.0 [options] element1 [property=value] ! element2 [property=value] ! element3 [property=value] ...
```

* `!` (感叹号): 连接符。相当于`Linux`里的`|`（管道符），把左边元件的输出（`Source Pad`）连接到右边元件的输入（`Sink Pad`）。
* `属性名=属性值`: 为前面的元件设置参数。多个参数用空格隔开。

### 示例

```bash
gst-launch-1.0 videotestsrc ! autovideosink
```

生成一个视频测试画面并显示，`videotestsrc`是源，生成彩条画面；`autovideosink`是终点，自动寻找合适的窗口显示出来。

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! videoconvert ! autovideosink
```

`videoconvert`，这是个万能的格式转换插件。因为摄像头吐出的格式（比如`YUY2`）显示器可能不认识，`videoconvert`会自动在中间做格式转换。

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! autovideosink
```

强制摄像头以特定的分辨率和帧率输出，需要用到`caps`（能力过滤）

```bash
gst-launch-1.0 v4l2src num-buffers=150 ! videoconvert ! x264enc ! mp4mux ! filesink location=test.mp4
```

保存视频到文件（编码封装）。`num-buffers=150`限制只采集150帧，避免无限录制。流程：采集 -> 格式转换 ->`H.264`编码 -> 封装成`MP4` -> 写入本地文件。

## GST_DEBUG

`GST_DEBUG` 是一个环境变量，用于控制 GStreamer 的调试输出。通过设置这个变量，可以获取更详细的日志信息，帮助开发者诊断和调试问题。

日志级别从0到9，常用的有：

* 2（WARNING）：警告信息，表示可能存在问题但不影响程序运行。
* 3（ERROR）：错误信息，表示程序遇到了严重问题，可能无法继续运行。
* 4（FIXME）：表示代码中存在需要修复的问题。

```bash
GST_DEBUG=3 gst-launch-1.0 videotestsrc ! autovideosink
```

## gst-discoverer-1.0

`gst-discoverer-1.0` 是一个命令行工具，用于分析媒体文件的内容和结构。它可以提取媒体文件的元数据、流信息、编解码器信息等，帮助开发者了解媒体文件的详细信息。

```bash
gst-discoverer-1.0 test.mp4
```
