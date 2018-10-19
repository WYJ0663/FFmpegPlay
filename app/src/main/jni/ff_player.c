//
// Created by yijunwu on 2018/10/19.
//


#include <pthread.h>
#include <libavutil/time.h>
#include "ff_player.h"
#include "log.h"
#include "ff_audio.h"

Player *ffp_create_player() {
    Player *player = av_malloc(sizeof(Player));

    player->video = av_malloc(sizeof(Video));
    player->audio = av_malloc(sizeof(Audio));

    player->packet = av_packet_alloc();

    //音频
    player->audio->avPacket = av_packet_alloc();
    player->audio->avFrame = av_frame_alloc();

    player->audio->queue = createQueue();
    player->video->queue = createQueue();

    player->audio->clock = 0;
    //初始化锁
    pthread_mutex_init(&player->audio->mutex, NULL);
    pthread_cond_init(&player->audio->cond, NULL);

    return player;
}


//
void ffp_init_ffmpeg(Player *player, char *url) {
    player->inputPath = url;

    LOGE("开启解码线程");
    //1.注册组件
//    av_register_all();
//    avformat_network_init();
    //封装格式上下文
    player->pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&player->pFormatCtx, player->inputPath, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(player->pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
    }

    //得到播放总时间
    if (player->pFormatCtx->duration != AV_NOPTS_VALUE) {
        player->duration = player->pFormatCtx->duration;//微秒
        // todo
//        set_total_time_callback(duration);
    }

    //找到视频流和音频流
    for (int i = 0; i < player->pFormatCtx->nb_streams; ++i) {
        //获取解码器
        AVCodec *avCodec = avcodec_find_decoder(player->pFormatCtx->streams[i]->codecpar->codec_id);
        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);
        avcodec_parameters_to_context(codecContext, player->pFormatCtx->streams[i]->codecpar);

        if (avcodec_open2(codecContext, avCodec, NULL) < 0) {
            LOGE("打开失败")
            continue;
        }
        //如果是视频流
        if (player->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            player->video->index = i;
            player->video->codec = codecContext;
            player->video->time_base = player->pFormatCtx->streams[i]->time_base;

            // todo
//            set_window_buffers_geometry();
        }//如果是音频流
        else if (player->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            player->audio->index = i;
            player->audio->codec = codecContext;
            player->audio->time_base = player->pFormatCtx->streams[i]->time_base;
        }
    }

    ffp_init_audio_ffmpeg(player->audio);
    create_openelse(player->audio);
}


int ffp_init_audio_ffmpeg(Audio *audio) {
    LOGE("初始化ffmpeg");
    audio->swrContext = swr_alloc();

    int length = 0;
    int got_frame;
    //    44100*2
    audio->out_buffer = (uint8_t *) av_mallocz(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
    //输出的采样率必须与输入相同
    int out_sample_rate = audio->codec->sample_rate;
    swr_alloc_set_opts(audio->swrContext, out_ch_layout, out_formart, out_sample_rate,
                       audio->codec->channel_layout, audio->codec->sample_fmt, audio->codec->sample_rate, 0,
                       NULL);

    swr_init(audio->swrContext);
    //    获取通道数  2
    audio->out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("------>通道数%d  ", audio->out_channer_nb);

    //倍速初始化
    audio->out_rate_buffer = (short *) av_mallocz(44100 * 2);
    audio->sonic = sonicCreateStream(audio->codec->sample_rate, audio->out_channer_nb);
    return 0;
}


void *start_play(void *arg) {
    Player *player = (Player *) arg;
    //开启播放
    //seekTo(0);
    //解码packet,并压入队列中
    AVPacket *packet = player->packet;
    //跳转到某一个特定的帧上面播放
    int ret;
    while (player->isPlay) {

        ret = av_read_frame(player->pFormatCtx, player->packet);
        LOGE("av_read_frame %d  %d %d", packet->stream_index, player->audio->index, ret);
        if (ret == 0) {
            if (player->video && player->video->isPlay && packet->stream_index == player->video->index) {
                //将视频packet压入队列

//                checkQueue();

            } else if (packet->stream_index == player->audio->index) {
                LOGE("put_packet");
                put_packet(player->audio);

//                checkQueue();
            }
        } else if (ret == AVERROR_EOF) {
            // 读完了
            //读取完毕 但是不一定播放完毕
            while (player->isPlay) {
//                if (player->audio->queue->size <= 0 && player->audio->queue->size <= 0) {
//                    break;
//                }
                // LOGE("等待播放完成");
                av_usleep(10000);
            }
        }
        av_packet_unref(packet);
        av_init_packet(packet);
    }
    //解码完过后可能还没有播放完

//    if (ffmpegMusic && ffmpegMusic->isPlay) {
//        ffmpegMusic->stop();
//    }
//    if (ffmpegVideo && ffmpegVideo->isPlay) {
//        ffmpegVideo->stop();
//    }
    //释放

//    avformat_close_input(&pFormatCtx);
//    avformat_free_context(pFormatCtx);
}


void ffp_play(Player *player) {
    player->isPlay = true;
    player->audio->isPlay = true;
    player->video->isPlay = true;
    pthread_create(&player->p_id, NULL, start_play, player);

    pthread_create(&player->audio->p_id, NULL, ffp_start_audio_play, player->audio);
}

void ffp_stop(Player *player) {
    player->isPlay = false;
    av_packet_unref(player->packet);
    av_packet_free(&player->packet);
    avformat_close_input(&player->pFormatCtx);
//    avformat_free_context(pFormatCtx);
}