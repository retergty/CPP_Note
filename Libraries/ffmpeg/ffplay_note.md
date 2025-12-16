# ffplay 源码分析

ffplay是FFmpeg套件中的一个简单的多媒体播放器，主要用于播放音频和视频文件。它基于FFmpeg库，支持多种格式和编解码器。

ffplay是多线程程序，主要包括以下几个线程：

* 主线程：负责初始化、事件处理和UI更新。
* 读取线程：负责从输入文件或流中读取Packet数据并使用队列发送出去。
* 解码线程：负责音频和视频的解码。
    * 音频解码线程：从队列接受音频Packet并解码成PCM格式。
    * 视频解码线程：从队列接受视频Packet并解码成YUV格式。
    * 字幕解码线程：从队列接受字幕Packet并解码成文本格式。
* 渲染线程：负责视频的显示和音频的播放。

## 关键结构体

### `VideoState`

`VideoState`结构体是ffplay的核心数据结构，包含了播放器的所有状态信息，包括音频和视频流的解码器、缓冲区、同步信息等。

#### 主要成员变量

* `AVFormatContext *ic`：输入文件的格式上下文。

* `SDL_Thread *read_tid;`：读取线程的线程ID。

* `Decoder auddec`：音频解码器。
* `Decoder viddec`：视频解码器。
* `Decoder subdec`：字幕解码器。

* `FrameQueue pictq;`：视频帧队列。
* `FrameQueue sampq;`：音频帧队列。
* `FrameQueue subpq;`：字幕帧队列。

* `PacketQueue audioq;`：音频Packet队列。
* `PacketQueue videoq;`：视频Packet队列。
* `PacketQueue subtitleq;`：字幕Packet队列。

## main函数分析

### 处理命令行参数

处理命令行参数，设置输入文件路径和各种选项。

### SDL子系统初始化

进行SDL子系统的初始化，包括视频、音频和定时器等模块。

```CPP
if (display_disable) {
    video_disable = 1;
}
flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
if (audio_disable)
    flags &= ~SDL_INIT_AUDIO;
else {
    /* Try to work around an occasional ALSA buffer underflow issue when the
    * period size is NPOT due to ALSA resampling by forcing the buffer size. */
    if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
        SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
    }
if (display_disable)
    flags &= ~SDL_INIT_VIDEO;
if (SDL_Init (flags)) {
    av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
    av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
    exit(1);
}
```

初始化了SDL的各个子系统，根据用户的选项决定是否启用音频和视频模块。如果初始化失败，程序会输出错误信息并退出。

```CPP
if (!display_disable) {
    int flags = SDL_WINDOW_HIDDEN;
    if (alwaysontop)
#if SDL_VERSION_ATLEAST(2,0,5)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#else
        av_log(NULL, AV_LOG_WARNING, "Your SDL version doesn't support SDL_WINDOW_ALWAYS_ON_TOP. Feature will be inactive.\n");
#endif
    if (borderless)
        flags |= SDL_WINDOW_BORDERLESS;
    else
        flags |= SDL_WINDOW_RESIZABLE;

#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
    if (hwaccel && !enable_vulkan) {
        av_log(NULL, AV_LOG_INFO, "Enable vulkan renderer to support hwaccel %s\n", hwaccel);
        enable_vulkan = 1;
    }
    if (enable_vulkan) {
        vk_renderer = vk_get_renderer();
        if (vk_renderer) {
#if SDL_VERSION_ATLEAST(2, 0, 6)
            flags |= SDL_WINDOW_VULKAN;
#endif
        } else {
            av_log(NULL, AV_LOG_WARNING, "Doesn't support vulkan renderer, fallback to SDL renderer\n");
            enable_vulkan = 0;
        }
    }
    window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (!window) {
        av_log(NULL, AV_LOG_FATAL, "Failed to create window: %s", SDL_GetError());
        do_exit(NULL);
    }

    if (vk_renderer) {
        AVDictionary *dict = NULL;

        if (vulkan_params) {
            int ret = av_dict_parse_string(&dict, vulkan_params, "=", ":", 0);
            if (ret < 0) {
                av_log(NULL, AV_LOG_FATAL, "Failed to parse, %s\n", vulkan_params);
                do_exit(NULL);
            }
        }
        ret = vk_renderer_create(vk_renderer, window, dict);
        av_dict_free(&dict);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to create vulkan renderer, %s\n", av_err2str(ret));
            do_exit(NULL);
        }
    } else {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, 0);
        }
        if (renderer) {
            if (!SDL_GetRendererInfo(renderer, &renderer_info))
                av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", renderer_info.name);
        }
        if (!renderer || !renderer_info.num_texture_formats) {
            av_log(NULL, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
            do_exit(NULL);
        }
    }
}
```

创建SDL窗口和渲染器，根据用户选项设置窗口属性（如是否无边框、是否总在最前等）。如果启用了Vulkan渲染器，则尝试创建Vulkan渲染器，否则使用SDL的默认渲染器。

### 初始化VideoState

```CPP
is = stream_open(input_filename, file_iformat);
if (!is) {
    av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
    do_exit(NULL);
}
```

调用`stream_open`函数初始化`VideoState`结构体，打开输入文件并准备解码器和队列。如果初始化失败，程序会输出错误信息并退出。

## read_thread函数分析

`read_thread`函数是ffplay中负责读取输入文件数据的线程函数。它不断从输入文件中读取Packet数据，并将其分发到相应的解码队列中。

`read_thread`函数的主要逻辑如下：

1. 循环读取Packet数据，直到遇到结束标志或错误。
2. 根据Packet的流索引，将Packet分发到音频、视频或字幕的Packet队列中。
3. 处理特殊情况，如暂停状态、缓冲区满等。

### 使用stream_component_open函数打开流组件（音频、视频、字幕）。

```CPP
if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    ret = create_hwaccel(&avctx->hw_device_ctx);
    if (ret < 0)
        goto fail;
}
```

如果流类型是视频，调用`create_hwaccel`函数创建硬件加速上下文，以提高视频解码性能。

### 处理Packet数据

```CPP
ret = av_read_frame(ic, pkt);
if (ret < 0) {
    if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
        if (is->video_stream >= 0)
            packet_queue_put_nullpacket(&is->videoq, pkt, is->video_stream);
        if (is->audio_stream >= 0)
            packet_queue_put_nullpacket(&is->audioq, pkt, is->audio_stream);
        if (is->subtitle_stream >= 0)
            packet_queue_put_nullpacket(&is->subtitleq, pkt, is->subtitle_stream);
        is->eof = 1;
    }
    if (ic->pb && ic->pb->error) {
        if (autoexit)
            goto fail;
        else
            break;
    }
    SDL_LockMutex(wait_mutex);
    SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
    SDL_UnlockMutex(wait_mutex);
    continue;
} else {
    is->eof = 0;
}

if (show_status && ic->streams[pkt->stream_index]->event_flags &
    AVSTREAM_EVENT_FLAG_METADATA_UPDATED) {
    fprintf(stderr, "\x1b[2K\r");
    snprintf(metadata_description,
                sizeof(metadata_description),
                "\r  New metadata for stream %d",
                pkt->stream_index);
    dump_dictionary(NULL, ic->streams[pkt->stream_index]->metadata,
                        metadata_description, "    ", AV_LOG_INFO);
}
ic->streams[pkt->stream_index]->event_flags &= ~AVSTREAM_EVENT_FLAG_METADATA_UPDATED;

/* check if packet is in play range specified by user, then queue, otherwise discard */
stream_start_time = ic->streams[pkt->stream_index]->start_time;
pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
pkt_in_play_range = duration == AV_NOPTS_VALUE ||
        (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
        av_q2d(ic->streams[pkt->stream_index]->time_base) -
        (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
        <= ((double)duration / 1000000);
if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
    packet_queue_put(&is->audioq, pkt);
} else if (pkt->stream_index == is->video_stream && pkt_in_play_range
            && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
    packet_queue_put(&is->videoq, pkt);
} else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
    packet_queue_put(&is->subtitleq, pkt);
} else {
    av_packet_unref(pkt);
}
```

根据读取到的Packet的流索引，将其放入对应的Packet队列中（音频、视频或字幕）。如果Packet不在播放范围内，则释放该Packet。

## video_thread函数分析

`video_thread`函数是ffplay中负责视频解码和渲染的线程函数。它从视频Packet队列中获取Packet数据，进行解码，并将解码后的帧发送到视频帧队列中。

### 接受视频Packet并解码成AVFrame

```CPP
static int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub) {
    int ret = AVERROR(EAGAIN);

    for (;;) {
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request)
                    return -1;

                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            if (decoder_reorder_pts == -1) {
                                frame->pts = frame->best_effort_timestamp;
                            } else if (!decoder_reorder_pts) {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            AVRational tb = (AVRational){1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                }
                if (ret == AVERROR_EOF) {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        do {
            if (d->queue->nb_packets == 0)
                SDL_CondSignal(d->empty_queue_cond);
            if (d->packet_pending) {
                d->packet_pending = 0;
            } else {
                int old_serial = d->pkt_serial;
                if (packet_queue_get(d->queue, d->pkt, 1, &d->pkt_serial) < 0)
                    return -1;
                if (old_serial != d->pkt_serial) {
                    avcodec_flush_buffers(d->avctx);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
            }
            if (d->queue->serial == d->pkt_serial)
                break;
            av_packet_unref(d->pkt);
        } while (1);

        if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            int got_frame = 0;
            ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, d->pkt);
            if (ret < 0) {
                ret = AVERROR(EAGAIN);
            } else {
                if (got_frame && !d->pkt->data) {
                    d->packet_pending = 1;
                }
                ret = got_frame ? 0 : (d->pkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
            }
            av_packet_unref(d->pkt);
        } else {
            if (d->pkt->buf && !d->pkt->opaque_ref) {
                FrameData *fd;

                d->pkt->opaque_ref = av_buffer_allocz(sizeof(*fd));
                if (!d->pkt->opaque_ref)
                    return AVERROR(ENOMEM);
                fd = (FrameData*)d->pkt->opaque_ref->data;
                fd->pkt_pos = d->pkt->pos;
            }

            if (avcodec_send_packet(d->avctx, d->pkt) == AVERROR(EAGAIN)) {
                av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                d->packet_pending = 1;
            } else {
                av_packet_unref(d->pkt);
            }
        }
    }
}
```

`decoder_decode_frame`函数从队列中获取Packet，并调用`avcodec_send_packet`和`avcodec_receive_frame`进行解码，返回解码后的`AVFrame`。

### 音画同步

```CPP
if (got_picture) {
    double dpts = NAN;

    if (frame->pts != AV_NOPTS_VALUE)
        dpts = av_q2d(is->video_st->time_base) * frame->pts;

    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

    if (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
        if (frame->pts != AV_NOPTS_VALUE) {
            double diff = dpts - get_master_clock(is);
            if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                diff - is->frame_last_filter_delay < 0 &&
                is->viddec.pkt_serial == is->vidclk.serial &&
                is->videoq.nb_packets) {
                is->frame_drops_early++;
                av_frame_unref(frame);
                got_picture = 0;
            }
        }
    }
}
```

这一段代码实现了音画同步的逻辑，通过比较视频帧的PTS和主时钟的时间差，决定是否丢弃当前帧以保持同步。

### 应用滤镜并送入视频帧队列

```CPP
while (ret >= 0) {
    FrameData *fd;

    is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

    ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
    if (ret < 0) {
        if (ret == AVERROR_EOF)
            is->viddec.finished = is->viddec.pkt_serial;
        ret = 0;
        break;
    }

    fd = frame->opaque_ref ? (FrameData*)frame->opaque_ref->data : NULL;

    is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
    if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
        is->frame_last_filter_delay = 0;
    tb = av_buffersink_get_time_base(filt_out);
    duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
    pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    ret = queue_picture(is, frame, pts, duration, fd ? fd->pkt_pos : -1, is->viddec.pkt_serial);
    av_frame_unref(frame);
    if (is->videoq.serial != is->viddec.pkt_serial)
        break;
}
```

将解码后的`AVFrame`通过滤镜处理后，送入视频帧队列`pictq`，以供后续渲染使用。

计算了帧的PTS和持续时间，并调用`queue_picture`函数将帧加入队列。

## audio_thread函数分析

`audio_thread`函数是ffplay中负责音频解码和播放的线程函数。它从音频Packet队列中获取Packet数据，进行解码，并将解码后的PCM数据发送到音频设备进行播放。

这个函数比较简单，和`video_thread`不同，它主要关注音频数据的解码和播放，而不涉及复杂的同步和渲染逻辑，因为音频本身就是参考时钟。

### 重新配置滤镜

如果音频参数发生变化，重新配置滤镜图。

```CPP
reconfigure =
    cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.ch_layout.nb_channels,
                    frame->format, frame->ch_layout.nb_channels)    ||
    av_channel_layout_compare(&is->audio_filter_src.ch_layout, &frame->ch_layout) ||
    is->audio_filter_src.freq           != frame->sample_rate ||
    is->auddec.pkt_serial               != last_serial;

if (reconfigure) {
    char buf1[1024], buf2[1024];
    av_channel_layout_describe(&is->audio_filter_src.ch_layout, buf1, sizeof(buf1));
    av_channel_layout_describe(&frame->ch_layout, buf2, sizeof(buf2));
    av_log(NULL, AV_LOG_DEBUG,
            "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
            is->audio_filter_src.freq, is->audio_filter_src.ch_layout.nb_channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
            frame->sample_rate, frame->ch_layout.nb_channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

    is->audio_filter_src.fmt            = frame->format;
    ret = av_channel_layout_copy(&is->audio_filter_src.ch_layout, &frame->ch_layout);
    if (ret < 0)
        goto the_end;
    is->audio_filter_src.freq           = frame->sample_rate;
    last_serial                         = is->auddec.pkt_serial;

    if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
        goto the_end;
}
```

如果音频帧的格式、采样率或通道布局发生变化，重新配置音频滤镜图以适应新的音频参数。

### 应用滤镜并送入音频帧队列

```CPP
if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
    goto the_end;

while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
    FrameData *fd = frame->opaque_ref ? (FrameData*)frame->opaque_ref->data : NULL;
    tb = av_buffersink_get_time_base(is->out_audio_filter);
    if (!(af = frame_queue_peek_writable(&is->sampq)))
        goto the_end;

    af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    af->pos = fd ? fd->pkt_pos : -1;
    af->serial = is->auddec.pkt_serial;
    af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

    av_frame_move_ref(af->frame, frame);
    frame_queue_push(&is->sampq);

    if (is->audioq.serial != is->auddec.pkt_serial)
        break;
}
```
