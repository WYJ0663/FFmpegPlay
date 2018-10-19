//
// Created by yijunwu on 2018/10/11.
//

#include "Player.h"
#include <pthread.h>

static void (*set_window_buffers_geometry)();

static void (*set_total_time_callback)(int64_t duration);

bool Player::checkIsPlay() {
    return isPlay && NULL != ffmpegVideo && NULL != ffmpegMusic && ffmpegVideo->isPlay && ffmpegMusic->isPlay;
}

void Player::initFFmpeg() {
    LOGE("开启解码线程")
    //1.注册组件
//    av_register_all();
//    avformat_network_init();
    //封装格式上下文
    pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, inputPath, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
    }

    //得到播放总时间
    if (pFormatCtx->duration != AV_NOPTS_VALUE) {
        duration = pFormatCtx->duration;//微秒
        set_total_time_callback(duration);
    }
}

void Player::init(const char *inputPath) {
    this->inputPath = inputPath;
    initFFmpeg();

    ffmpegVideo = new FFmpegVideo;
    ffmpegMusic = new FFmpegMusic;

//    ffmpegVideo->setPlayCall(call_video_play);
//    ffmpegMusic->setPlayCall(call_music_play);
//    pthread_create(&p_tid, NULL, begin, NULL);//开启begin线程

}


Player::~Player() {
    if (ffmpegVideo) {
        delete (ffmpegVideo);
        ffmpegVideo = 0;
    }
    if (ffmpegMusic) {
        delete (ffmpegMusic);
        ffmpegMusic = 0;
    }

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}


void Player::stop() {//释放资源
    if (isPlay) {
        isPlay = false;
    }
    if (ffmpegVideo) {
        if (ffmpegVideo->isPlay) {
            ffmpegVideo->stop();
        }
    }
    if (ffmpegMusic) {
        if (ffmpegMusic->isPlay) {
            ffmpegMusic->stop();
        }
    }
}

void *freeQ(void *args) {
    Queue *queue = (Queue *) args;
    LOGE("1 queue size %d", queue->size);
    cleanQueue(queue);
    freeQueue(queue);
    return NULL;
}

//单位秒
void Player::seekTo(int mesc) {
    if (!checkIsPlay()) {
        return;
    }

    if (mesc <= 0) {
        mesc = 0;
    }

    //清空vector
//    seekCleanQueue(ffmpegMusic->queue, &ffmpegMusic->mutex, &ffmpegMusic->cond);
//    seekCleanQueue(ffmpegVideo->queue, &ffmpegVideo->mutex, &ffmpegVideo->cond);

//    Queue *queue1 = ffmpegMusic->queue;//销毁
//    Queue *queue2 = ffmpegMusic->queue;
//    ffmpegMusic->queue = createQueue();
//    ffmpegVideo->queue = createQueue();

    Queue *queue1;
    Queue *queue2;
    pthread_mutex_lock(&ffmpegMusic->mutex);
    queue1 = ffmpegMusic->queue;
    ffmpegMusic->queue = createQueue();
    pthread_mutex_unlock(&ffmpegMusic->mutex);

    pthread_mutex_lock(&ffmpegVideo->mutex);
    queue2 = ffmpegVideo->queue;
    ffmpegVideo->queue = createQueue();
    LOGE("queue size %d", queue2->size);
    pthread_mutex_unlock(&ffmpegVideo->mutex);

    LOGE("av_seek_frame");
    av_seek_frame(pFormatCtx, ffmpegVideo->index, (int64_t) (mesc / av_q2d(ffmpegVideo->time_base)),
                  AVSEEK_FLAG_BACKWARD);
    av_seek_frame(pFormatCtx, ffmpegMusic->index, (int64_t) (mesc / av_q2d(ffmpegMusic->time_base)),
                  AVSEEK_FLAG_BACKWARD);
    startQueue();
//
    pthread_t p_tid1;
    pthread_t p_tid2;
    LOGE("2 queue size %d", queue2->size);
    pthread_create(&p_tid1, NULL, freeQ, queue1);
    int rc = pthread_create(&p_tid2, NULL, freeQ, queue2);
    LOGE("ERROR; return code is %d\n", rc);

}

void Player::play() {

    //找到视频流和音频流
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        //获取解码器
        AVCodec *avCodec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);
        avcodec_parameters_to_context(codecContext, pFormatCtx->streams[i]->codecpar);

//        AVCodecContext *avCodecContext = pFormatCtx->streams[i]->codec;
//        AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
//
//        //copy一个解码器，
//        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);
//        avcodec_copy_context(codecContext, avCodecContext);
        if (avcodec_open2(codecContext, avCodec, NULL) < 0) {
            LOGE("打开失败")
            continue;
        }
        //如果是视频流
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ffmpegVideo->index = i;
            ffmpegVideo->setAvCodecContext(codecContext);
            ffmpegVideo->time_base = pFormatCtx->streams[i]->time_base;

            set_window_buffers_geometry();
        }//如果是音频流
        else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            ffmpegMusic->index = i;
            ffmpegMusic->setAvCodecContext(codecContext);
            ffmpegMusic->time_base = pFormatCtx->streams[i]->time_base;
        }
    }
    //开启播放
    ffmpegVideo->setFFmepegMusic(ffmpegMusic);
    ffmpegMusic->play();
    ffmpegVideo->play();
    isPlay = true;
    //seekTo(0);
    //解码packet,并压入队列中
    packet = av_packet_alloc();
    //跳转到某一个特定的帧上面播放
    int ret;
    while (isPlay) {
        LOGE("av_read_frame");
        ret = av_read_frame(pFormatCtx, packet);
        if (ret == 0) {
            if (ffmpegVideo && ffmpegVideo->isPlay && packet->stream_index == ffmpegVideo->index) {
                //将视频packet压入队列
                ffmpegVideo->put(packet);

                checkQueue();

            } else if (ffmpegMusic && ffmpegMusic->isPlay &&
                       packet->stream_index == ffmpegMusic->index) {
                ffmpegMusic->put(packet);

                checkQueue();
            }
        } else if (ret == AVERROR_EOF) {
            // 读完了
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (ffmpegVideo->queue->size <= 0 && ffmpegMusic->queue->size <= 0) {
                    break;
                }
                // LOGE("等待播放完成");
                av_usleep(10000);
            }
        }
        av_packet_unref(packet);
        av_init_packet(packet);
    }
    //解码完过后可能还没有播放完
    isPlay = false;
    if (ffmpegMusic && ffmpegMusic->isPlay) {
        ffmpegMusic->stop();
    }
    if (ffmpegVideo && ffmpegVideo->isPlay) {
        ffmpegVideo->stop();
    }
    //释放
    av_packet_unref(packet);
    av_packet_free(&packet);
    avformat_close_input(&pFormatCtx);
//    avformat_free_context(pFormatCtx);
    pthread_exit(0);
}


void Player::startQueue() {
    uint32_t size = ffmpegMusic->queue->size;
    uint32_t size2 = ffmpegVideo->queue->size;
    if (size < MIN_QUEUE_SIZE || size2 < MIN_QUEUE_SIZE) {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}

void Player::checkQueue() {
    uint32_t size = ffmpegMusic->queue->size;
    uint32_t size2 = ffmpegVideo->queue->size;
    if (size > MAX_QUEUE_SIZE && size2 > MAX_QUEUE_SIZE) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
    }
}

void Player::pause() {
    if (checkIsPlay()) {
        ffmpegMusic->pause();
        ffmpegVideo->pause();
    }
}

void Player::silence() {
    if (checkIsPlay()) {
        if (ffmpegMusic->isSilence) {
            ffmpegMusic->isSilence = false;
        } else {
            ffmpegMusic->isSilence = true;
        }
    }
}

void Player::setRate(float rate) {
    LOGE("play music->rate 1 =%f", ffmpegMusic->rate)
    if (checkIsPlay()) {
        ffmpegMusic->rate = rate;
        LOGE("play music->rate=%f", ffmpegMusic->rate)
    }
}

void Player::cutImage() {
    isCutImage = true;
}

void Player::setWindowCallback(void (*call)()) {
    set_window_buffers_geometry = call;
}

void Player::setTotalTimeCallback(void (*call)(int64_t)) {
    set_total_time_callback = call;
}

Player::Player() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}
