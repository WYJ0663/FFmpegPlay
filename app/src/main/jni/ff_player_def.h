//
// Created by yijunwu on 2018/10/19.
//
#ifndef FF_PLAYER_DEF_H
#define FF_PLAYER_DEF_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/native_window.h>

#include "ff_packet_queue.h"
#include "sonic/sonic.h"
#include "gles2/egl.h"
#include "gles_draw.h"

#define bool int
#define false 0
#define true 1

typedef struct Player Player;
typedef struct AndroidJNI AndroidJNI;
typedef struct Video Video;
typedef struct Audio Audio;
typedef struct PlayerStatus PlayerStatus;

struct Player {
    char *inputPath;
    pthread_t p_id;

    Video *video;
    Audio *audio;

    int64_t duration;
    AVFormatContext *pFormatCtx;

    bool isCutImage;

    //同步锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

    AndroidJNI *androidJNI;

    PlayerStatus *status;

    int seekTime;//秒

    EGLContexts *eglContexts;
    GLESContexts *glesContexts;
};

struct PlayerStatus {
    bool isPlay;
    bool isPause;//是否暂停
};

struct AndroidJNI {
    //Android
    JavaVM *pJavaVM;
    jobject pInstance;
    bool isSize;
    double preClock;

    ANativeWindow *window;
};

struct Video {
    int index;//流索引
    pthread_t p_id;//处理线程
    Queue *queue;//队列

    int width;
    int height;

    AVCodecContext *codec;//解码器上下文

    //同步锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

    Audio *audio;

    AVRational time_base;
    double clock; //当前播放的时间

//    AVPacket *avPacket;
    AndroidJNI *androidJNI;

    PlayerStatus *status;
};


struct Audio {
    int index;//流索引

    bool isSilence;//静音

    AVPacket *avPacket;
    AVFrame *avFrame;

    pthread_t p_id;//处理线程
    Queue *queue;//队列
    // std::queue<AVPacket*> queueNull;//空队列
    AVCodecContext *codec;//解码器上下文

    SwrContext *swrContext;
    uint8_t *out_buffer;
    int out_channer_nb;

    //同步锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

    double clock;//从第一zhen开始所需要时间

    AVRational time_base;

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

    //倍速
    float rate;
    sonicStream sonic;
    short *out_rate_buffer;

    AndroidJNI *androidJNI;

    PlayerStatus *status;
};

#endif //FF_PLAYER_DEF_H
