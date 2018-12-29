//
// Created by yijunwu on 2018/10/19.
//

#include <pthread.h>
#include <libavutil/time.h>
#include "ff_player.h"
#include "log.h"
#include "ff_audio.h"
#include "ff_video.h"
#include "android_jni.h"

void init_param(Player *player);

Player *ffp_create_player() {
    Player *player = malloc(sizeof(Player));

    //音频
    player->audio = malloc(sizeof(Audio));
    player->audio->avFrame = av_frame_alloc();
    player->audio->queue = createQueue();

    //视频
    player->video = malloc(sizeof(Video));
    player->video->width = 0;
    player->video->height = 0;
    player->video->queue = createQueue();
    player->video->audio = player->audio;

    //Android
    player->androidJNI = malloc(sizeof(AndroidJNI));
    player->audio->androidJNI = player->androidJNI;
    player->video->androidJNI = player->androidJNI;

    //状态
    player->status = malloc(sizeof(PlayerStatus));
    player->audio->status = player->status;
    player->video->status = player->status;
    pthread_mutex_init(&player->status->mutex, NULL);
    pthread_cond_init(&player->status->cond, NULL);

    //opengles、egl
    player->eglContexts = malloc(sizeof(EGLContexts));
    player->glesContexts = malloc(sizeof(GLESContexts));

    //参数初始化
    init_param(player);

    return player;
}

void init_param(Player *player) {
    player->isCutImage = false;
    player->video->clock = 0;
    player->androidJNI->preClock = 0;
    player->androidJNI->isSize = false;

    player->status->isPlay = false;
    player->status->isPause = false;

    player->audio->clock = 0;
    player->audio->rate = 1;
    player->audio->isSilence = false;

    player->androidJNI->window = 0;

    player->seekTime = -1;

    player->eglContexts->eglDisplay = 0;
    player->eglContexts->eglSurface = 0;
    player->eglContexts->eglContext = 0;
    player->eglContexts->eglFormat = 0;//颜色格式
    player->eglContexts->config = 0;

    player->glesContexts->program = 0;
}


void ffp_stop(Player *player) {
    player->status->isPlay = false;
    av_usleep(100);
    //取消队列等待
    pthread_mutex_lock(&player->audio->queue->mutex);
    pthread_cond_signal(&player->audio->queue->cond);
    pthread_mutex_unlock(&player->audio->queue->mutex);

    pthread_mutex_lock(&player->video->queue->mutex);
    pthread_cond_signal(&player->video->queue->cond);
    pthread_mutex_unlock(&player->video->queue->mutex);

    pthread_mutex_lock(&player->status->mutex);
    pthread_cond_signal(&player->status->cond);
    pthread_mutex_unlock(&player->status->mutex);

//    av_usleep(1000);
    pthread_join(player->p_id, 0);
    pthread_join(player->audio->p_id, 0);
    pthread_join(player->video->p_id, 0);

    ffp_free(player);
}

void ffp_free(Player *player) {
    LOGE("回收内存");
    pthread_mutex_destroy(&player->status->mutex);
    pthread_cond_destroy(&player->status->cond);

    avformat_close_input(&player->pFormatCtx);

    av_frame_unref(player->audio->avFrame);
    av_frame_free(&player->audio->avFrame);
    freeAVPacketQueue(player->audio->queue);
    ffp_audio_free(player->audio);
    avcodec_free_context(&player->audio->codec);

    freeAVPacketQueue(player->video->queue);
    avcodec_free_context(&player->video->codec);;

    if (player->androidJNI->window) {
        ANativeWindow_release(player->androidJNI->window);
        player->androidJNI->window = 0;
    }

    JNIEnv *env;
    (*player->androidJNI->pJavaVM)->AttachCurrentThread(player->androidJNI->pJavaVM, &env, NULL);
    (*env)->DeleteGlobalRef(env, player->androidJNI->pInstance);
    player->androidJNI->pInstance = NULL;
//    (*player->androidJNI->pJavaVM)->DetachCurrentThread(player->androidJNI->pJavaVM);

    glesDestroye(player->glesContexts, player->video->width, player->video->height);
    eglDestroye(player->eglContexts);

    free(player->androidJNI);
    free(player->status);
    free(player->audio);
    free(player->video);
    free(player->glesContexts);
    free(player->eglContexts);
    free(player);

    LOGE("回收内存结束");
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
        return;
    }
    //3.获取视频信息
    if (avformat_find_stream_info(player->pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }

    //得到播放总时间
    if (player->pFormatCtx->duration != AV_NOPTS_VALUE) {
        player->duration = player->pFormatCtx->duration;//微秒
        set_total_time(player);
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
            player->video->width = codecContext->width;
            player->video->height = codecContext->height;
//            init_window(player);
            change_window_size(player);
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
    audio->out_buffer = (uint8_t *) malloc(44100 * 2);
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
    audio->out_rate_buffer = (short *) malloc(44100 * 2);
    audio->sonic = sonicCreateStream(audio->codec->sample_rate, audio->out_channer_nb);
    return 0;
}

void check_queue(Player *player) {
    if (player->video->queue->size > MAX_QUEUE_SIZE &&
        player->audio->queue->size > MAX_QUEUE_SIZE) {
        pthread_mutex_lock(&player->status->mutex);
        pthread_cond_wait(&player->status->cond, &player->status->mutex);
        pthread_mutex_unlock(&player->status->mutex);
    }
}

void *start_play(void *arg) {
    Player *player = (Player *) arg;
    AVPacket *avPacket = av_packet_alloc();
    int ret;
    while (player->status->isPlay) {
//        seek_to(player);
        ret = av_read_frame(player->pFormatCtx, avPacket);
        LOGE("av_read_frame %d  %d %d", avPacket->stream_index, player->audio->index, ret);
        if (ret == 0) {
            if (avPacket->stream_index == player->video->index) {
                //将视频packet压入队列
                put_video_packet(player->video, avPacket);
                check_queue(player);
            } else if (avPacket->stream_index == player->audio->index) {
                LOGE("put_packet");
                put_audio_packet(player->audio, avPacket);
                check_queue(player);
//                checkQueue();
            }
        } else if (ret == AVERROR_EOF) {
            // 读完了
            //读取完毕 但是不一定播放完毕
            while (player->status->isPlay) {
                if (player->audio->queue->size <= 0 && player->audio->queue->size <= 0) {
                    break;
                }
                // LOGE("等待播放完成");
                av_usleep(10000);
            }
        }
        av_packet_unref(avPacket);
    }
    (*player->androidJNI->pJavaVM)->DetachCurrentThread(player->androidJNI->pJavaVM);
    av_packet_unref(avPacket);
    av_packet_free(&avPacket);
    LOGE("退出线程 mian")
    pthread_exit(0);
}


void ffp_play(Player *player) {
    player->status->isPlay = true;
    pthread_create(&player->p_id, NULL, start_play, player);
    pthread_create(&player->audio->p_id, NULL, ffp_start_audio_play, player);
    pthread_create(&player->video->p_id, NULL, ffp_start_video_play, player);
}

void seek_to(Player *player) {
    LOGE("切进度 %d", player->seekTime)
    if (player->seekTime < 0) {
        return;
    }

    //清空vector
//    seekCleanQueue(ffmpegMusic->queue, &ffmpegMusic->mutex, &ffmpegMusic->cond);
//    seekCleanQueue(ffmpegVideo->queue, &ffmpegVideo->mutex, &ffmpegVideo->cond);

//    Queue *queue1 = ffmpegMusic->queue;//销毁
//    Queue *queue2 = ffmpegMusic->queue;
//    player->audio->queue = createQueue();
//    player->video->queue = createQueue();

    player->status->isPause = true;

    cleanAVPacketQueue(player->audio->queue);
    cleanAVPacketQueue(player->video->queue);

//    Queue *queue1;
//    Queue *queue2;
//    pthread_mutex_lock(&ffmpegMusic->mutex);
//    queue1 = ffmpegMusic->queue;
//    ffmpegMusic->queue = createQueue();
//    pthread_mutex_unlock(&ffmpegMusic->mutex);
//
//    pthread_mutex_lock(&ffmpegVideo->mutex);
//    queue2 = ffmpegVideo->queue;
//    ffmpegVideo->queue = createQueue();
//    LOGE("queue size %d", queue2->size);
//    pthread_mutex_unlock(&ffmpegVideo->mutex);

    pthread_mutex_lock(&player->audio->queue->mutex);
    pthread_mutex_lock(&player->video->queue->mutex);

    LOGE("av_seek_frame %d", (int) (player->seekTime / av_q2d(player->video->time_base)));
    av_seek_frame(player->pFormatCtx, player->video->index,
                  (int64_t) (player->seekTime / av_q2d(player->video->time_base)),
                  AVSEEK_FLAG_FRAME);
    av_seek_frame(player->pFormatCtx, player->audio->index,
                  (int64_t) (player->seekTime / av_q2d(player->audio->time_base)),
                  AVSEEK_FLAG_FRAME);

    player->seekTime = -1;
    player->status->isPause = false;

    pthread_cond_signal(&player->status->cond);

    pthread_mutex_unlock(&player->video->queue->mutex);
    pthread_mutex_unlock(&player->audio->queue->mutex);

//    startQueue();
//
//    pthread_t p_tid1;
//    pthread_t p_tid2;
//    LOGE("2 queue size %d", queue2->size);
//    pthread_create(&p_tid1, NULL, freeQ, queue1);
//    int rc = pthread_create(&p_tid2, NULL, freeQ, queue2);
//    LOGE("ERROR; return code is %d\n", rc);

}
