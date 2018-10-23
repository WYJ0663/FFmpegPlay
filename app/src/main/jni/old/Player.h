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
    AVPacket *avPacket;

    bool isCutImage = false;

    //同步锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

public:
    Player();

    ~Player();

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

    void checkQueue() ;

    void startQueue() ;
};
