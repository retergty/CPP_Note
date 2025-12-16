#include <iostream>
#include <fstream>
#include <vector>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

// 一个简单的辅助函数：把 YUV420P 的 Y 分量保存为 PGM 图片
// PGM 是最简单的灰度图格式，如果不清楚可以暂且理解为"黑白照片"
void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, int frame_index)
{
    char filename[32];
    sprintf(filename, "frame%03d.pgm", frame_index);
    FILE *f = fopen(filename, "wb");
    // PGM 头部: P5 <宽> <高> <最大像素值>
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, frame_index);
    // 写入像素数据 (对于 YUV420P，data[0] 就是灰度信息)
    for (int i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
    std::cout << "Saved " << filename << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./decoder <video_path>" << std::endl;
        return -1;
    }

    const char *filename = argv[1];

    // --- 1. 解封装 (Demux) ---
    AVFormatContext *format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, filename, nullptr, nullptr) != 0)
    {
        std::cerr << "Could not open file." << std::endl;
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0)
    {
        std::cerr << "Could not find stream info." << std::endl;
        return -1;
    }

    // --- 2. 寻找视频流与解码器 ---
    int video_stream_index = -1;
    const AVCodec *codec = nullptr;
    AVCodecParameters *codec_params = nullptr;

    // 遍历所有流，找到类型为 AVMEDIA_TYPE_VIDEO 的
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            codec_params = format_ctx->streams[i]->codecpar;
            // 根据 ID 查找解码器 (例如根据 AV_CODEC_ID_H264 找到 H.264 解码器)
            codec = avcodec_find_decoder(codec_params->codec_id);
            break;
        }
    }

    if (video_stream_index == -1 || codec == nullptr)
    {
        std::cerr << "No video stream or decoder found." << std::endl;
        return -1;
    }

    // --- 3. 初始化解码器上下文 (Context) ---
    // 解码器上下文保存了解码过程中的状态
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

    // 将流的参数 (如分辨率、编码格式) 复制给解码器上下文
    avcodec_parameters_to_context(codec_ctx, codec_params);

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
    {
        std::cerr << "Could not open codec." << std::endl;
        return -1;
    }

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

                // 为了演示，我们只保存前 5 帧
                if (frame_count < 5)
                {
                    // YUV420P 中，data[0] 是 Y 分量 (灰度)，linesize[0] 是行宽
                    save_gray_frame(frame->data[0], frame->linesize[0], frame->width, frame->height, frame_count);
                    frame_count++;
                }

                // 每次用完 Frame 后，虽然 receive_frame 会自动 reset，但显式清理是个好习惯
                av_frame_unref(frame);
            }
        }

        // 每次用完 Packet，必须重置引用计数，否则内存泄漏
        av_packet_unref(packet);
    }

    // --- 8. 资源清理 ---
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}