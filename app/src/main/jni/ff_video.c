//
// Created by yijunwu on 2018/10/22.
//
#include "ff_video.h"
#include "android_jni.h"

void get_video_packet(Video *video, AVPacket *avPacket) {
    LOGE("拿包  %d ", video->queue->size);
    pthread_mutex_lock(&video->mutex);
    while (video->status->isPlay == true) {
        if (video->queue->size > 0 && !video->status->isPause) {
            //如果队列中有数据可以拿出来
            if (getQueue(video->queue, avPacket) == 0) {
                LOGE("没有拿包  ");
            }
            break;
        } else {
            pthread_cond_wait(&video->cond, &video->mutex);
        }
    }
    pthread_mutex_unlock(&video->mutex);
}

void put_video_packet(Video *video, AVPacket *avPacket) {
    LOGE("包  %d ", video->queue->size);
    putQueue(video->queue, avPacket, &video->mutex, &video->cond);
}

double synchronize(Video *video, AVFrame *frame, double play) {
    //clock是当前播放的时间位置
    if (play != 0)
        video->clock = play;
    else //pst为0 则先把pts设为上一帧时间
        play = video->clock;
    //可能有pts为0 则主动增加clock
    //frame->repeat_pict = 当解码时，这张图片需要要延迟多少
    //需要求出扩展延时：
    //extra_delay = repeat_pict / (2*fps) 显示这样图片需要延迟这么久来显示
    double repeat_pict = frame->repeat_pict;
    //使用AvCodecContext的而不是stream的
    double frame_delay = av_q2d(video->codec->time_base);
    //如果time_base是1,25 把1s分成25份，则fps为25
    //fps = 1/(1/25)
    double fps = 1 / frame_delay;
    //pts 加上 这个延迟 是显示时间
    double extra_delay = repeat_pict / (2 * fps);
    double delay = extra_delay + frame_delay;
//    LOGI("extra_delay:%f",extra_delay);
    video->clock += delay;
    return play;
}

void *ffp_start_video_play(void *args) {
    Player *player = (Player *) args;
    Video *video = player->video;
    //申请AVFrame
    AVFrame *frame = av_frame_alloc();//分配一个AVFrame结构体,AVFrame结构体一般用于存储原始数据，指向解码后的原始帧
    AVFrame *rgb_frame = av_frame_alloc();//分配一个AVFrame结构体，指向存放转换成rgb后的帧
    AVPacket *packet = av_packet_alloc();

    //缓存区
    uint8_t *out_buffer = (uint8_t *) av_mallocz((size_t) av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                                                                   video->codec->width,
                                                                                   video->codec->height, 1));
    //与缓存区相关联，设置rgb_frame缓存区
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer, AV_PIX_FMT_RGBA, video->codec->width,
                         video->codec->height, 1);

    LOGE("转换成rgba格式")
    video->swsContext = sws_getContext(video->codec->width, video->codec->height,
                                       video->codec->pix_fmt,
                                       video->codec->width, video->codec->height, AV_PIX_FMT_RGBA,
                                       SWS_BICUBIC, NULL, NULL, NULL);

    LOGE("LC XXXXX  %f", video->codec);

    double last_play  //上一帧的播放时间
    , play             //当前帧的播放时间
    , last_delay    // 上一次播放视频的两帧视频间隔时间
    , delay         //两帧视频间隔时间
    , audio_clock //音频轨道 实际播放时间
    , diff   //音频帧与视频帧相差时间
    , sync_threshold
    , start_time  //从第一帧开始的绝对时间
    , pts
    , actual_delay//真正需要延迟时间
    ;

    //从第一帧开始的绝对时间
    start_time = av_gettime() / 1000000.0;
    LOGE("解码 ")
    while (video->status->isPlay) {
        get_video_packet(video, packet);
//        LOGE("解码 %d", packet->stream_index)
        // 解码
        avcodec_send_packet(video->codec, packet);
        if (avcodec_receive_frame(video->codec, frame) != 0) {
            continue;
        }
//        avcodec_decode_video2(video->codec, frame, &frameCount, packet);
//        if (!frameCount) {
//            continue;
//        }
        //转换为rgb格式
        sws_scale(video->swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                  frame->height, rgb_frame->data,
                  rgb_frame->linesize);
        LOGE("frame 宽%d,高%d", frame->width, frame->height);
        LOGE("rgb格式 宽%d,高%d", rgb_frame->width, rgb_frame->height);

        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE) {
            pts = 0;
        }

        play = pts * av_q2d(video->time_base);
        //纠正时间
        play = synchronize(video, frame, play);

        delay = play - last_play;
        if (delay <= 0 || delay > 1) {
            delay = last_delay;
        }
        audio_clock = video->audio->clock;
        last_delay = delay;
        last_play = play;
        //音频与视频的时间差
        diff = video->clock - audio_clock;
        //在合理范围外  才会延迟  加快
        sync_threshold = (delay > 0.01 ? 0.01 : delay);

        if (fabs(diff) < 10) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }

        start_time += delay;
        actual_delay = start_time - av_gettime() / 1000000.0;
        if (actual_delay > 0.01) {
            av_usleep(actual_delay * 1000000.0 + 6000);
        }

        LOGE("播放视频");
        call_video_play(player, rgb_frame);
        av_packet_unref(packet);
//        av_frame_unref(rgb_frame);
        av_frame_unref(frame);
    }
    LOGE("free packet");
    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);
    av_frame_unref(rgb_frame);
    av_frame_free(&rgb_frame);
    av_free(out_buffer);
    sws_freeContext(video->swsContext);
    (*player->androidJNI->pJavaVM)->DetachCurrentThread(player->androidJNI->pJavaVM);
    LOGE("退出线程 video")
    pthread_exit(0);
}

