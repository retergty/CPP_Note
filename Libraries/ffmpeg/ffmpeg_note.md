# ffmpeg

## 概念

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

## 常用函数