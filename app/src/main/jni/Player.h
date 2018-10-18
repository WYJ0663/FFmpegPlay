//
// Created by yijunwu on 2018/10/11.
//

#include "FFmpegMusic.h"
#include "FFmpegVideo.h"
#include <android/native_window_jni.h>

class Player {
public:
    const char *inputPath;
    FFmpegVideo *ffmpegVideo;
    FFmpegMusic *ffmpegMusic;
//    pthread_t p_tid;
    bool isPlay;
    int64_t duration;
    AVFormatContext *pFormatCtx;
    AVPacket *packet;

    bool isCutImage = false;

public:
    void setWindowCallback(void (*call)());
    void setTotalTimeCallback(void (*call)(int64_t duration));

    void init(const char *inputPath);

    void play();

    void seekTo(int mesc);

    void stop();

    bool checkIsPlay();

    void initFFmpeg();

    void pause();

    void silence();

    void setRate(float rate);

    void cutImage();

};


