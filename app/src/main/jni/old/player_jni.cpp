#include <jni.h>
#include <string>
#include "FFmpegMusic.h"
#include "FFmpegVideo.h"
#include <android/native_window_jni.h>
#include <MemoryTrace.hpp>
#include <fstream>

#include "Player.h"

extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"

#include <unistd.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "../log.h"
}
char *inputPath;

Player *player;

JavaVM *pJavaVM;
jobject pInstance;
bool isSize = false;
double preClock;

pthread_t p_tid;

ANativeWindow *window = 0;

void changeSize(int width, int height) {

    if (isSize || NULL == pJavaVM || NULL == pInstance) {
        return;
    }

    JNIEnv *env;
    pJavaVM->AttachCurrentThread(&env, NULL);

    static jmethodID methodID;
    if (NULL != env && NULL == methodID) {
        jclass clazz = env->GetObjectClass(pInstance);
        methodID = env->GetMethodID(clazz, "changeSize", "(II)V");
        env->DeleteLocalRef(clazz);
    }

    if (NULL != methodID && width > 0 && height > 0) {
        env->CallVoidMethod(pInstance, methodID, width, height);
        LOGE("changeSize %d", width);
        isSize = true;
    } else {
        LOGE("methodID == null");
    }

    pJavaVM->DetachCurrentThread();
}

void setTotalTime(int64_t duration) {
    JNIEnv *env;
    pJavaVM->AttachCurrentThread(&env, NULL);

    static jmethodID methodID;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = env->GetObjectClass(pInstance);
        methodID = env->GetMethodID(clazz, "setTotalTime", "(I)V");
        env->DeleteLocalRef(clazz);
    }

    if (NULL != methodID && duration > 0) {
        LOGE("setTotalTime == null%d", (jint) (duration / 1000));
        env->CallVoidMethod(pInstance, methodID, (jint) (duration / 1000));
    } else {
        LOGE("methodID == null");
    }
    pJavaVM->DetachCurrentThread();
}

void setCurrentTime(double duration) {
    JNIEnv *env;
    pJavaVM->AttachCurrentThread(&env, NULL);

    static jmethodID methodID = NULL;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = env->GetObjectClass(pInstance);
        methodID = env->GetMethodID(clazz, "setCurrentTime", "(I)V");
        env->DeleteLocalRef(clazz);
    }

    if (NULL != methodID && duration > 0) {
        env->CallVoidMethod(pInstance, methodID, (jint) (duration * 1000));
    } else {
        LOGE("methodID == null");
    }
    pJavaVM->DetachCurrentThread();
}

void setCurrentImage(AVFrame *frame) {
    if (!player) {
        return;
    }

    if (!player->isCutImage) {
        return;
    }
    player->isCutImage = false;

    LOGE("setCurrentImage isCutImage=%d", player->isCutImage);
    JNIEnv *env;
    pJavaVM->AttachCurrentThread(&env, NULL);

    int w = player->ffmpegVideo->codec->width;
    int h = player->ffmpegVideo->codec->height;
    uint8_t *src = frame->data[0];
    int size = w * h;
//    uint8_t *drec = new uint8_t[size * 4];
//    memcpy(drec, src, size * 4);
    jintArray result = env->NewIntArray(size);
    env->SetIntArrayRegion(result, 0, size, reinterpret_cast<const jint *>(src));

    static jmethodID methodID = NULL;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = env->GetObjectClass(pInstance);
        methodID = env->GetMethodID(clazz, "setCurrentImage", "([III)V");
        env->DeleteLocalRef(clazz);
    }
    LOGE("setCurrentImage w=%d", w);
    if (NULL != methodID) {
        LOGE("setCurrentImage CallVoidMethod");
        env->CallVoidMethod(pInstance, methodID, result, w, h);
    } else {
        LOGE("setCurrentImage == null");
    }
//    env->ReleaseIntArrayElements(result, reinterpret_cast<jint *>(src), 0);
    pJavaVM->DetachCurrentThread();
}

void call_video_play(AVFrame *frame) {
    if (!window) {
        return;
    }
    setCurrentImage(frame);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }

    LOGE("绘制 宽%d,高%d", frame->width, frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ", window_buffer.width, window_buffer.height, frame->linesize[0]);

    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
//        LOGE("call_video_play i=%d", i)
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);

}

void call_music_play(double clock) {
    LOGE("call_music_play %lf %lf", clock, preClock);
    if ((clock - preClock) > 1) {
        preClock = clock;
        setCurrentTime(clock);
    }
}

void stop(JNIEnv *env) {//释放资源
    preClock = 0;
    if (player) {
        player->stop();
        delete (player);
        player = 0;
    }

    //jni接口
//    if (pInstance) {
//        env->DeleteGlobalRef(pInstance);
//        pInstance = 0;
//    }
//    p_tid->s
}


int w = 0;
int h = 0;

void initWindow() {
    if (window && player && player->ffmpegVideo && player->ffmpegVideo->codec) {
        if (w == player->ffmpegVideo->codec->width && h == player->ffmpegVideo->codec->height) {
            return;
        }
        w = player->ffmpegVideo->codec->width;
        h = player->ffmpegVideo->codec->height;
        ANativeWindow_setBuffersGeometry(window, player->ffmpegVideo->codec->width,
                                         player->ffmpegVideo->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
        changeSize(player->ffmpegVideo->codec->width, player->ffmpegVideo->codec->height);
    }
}

void parseCall() {
    if (player) {
        player->startQueue();
    }
}

void *begin(void *avg) {
    player = new Player;
    player->ffmpegVideo->setPlayCall(call_video_play);
    player->ffmpegMusic->setPlayCall(call_music_play);
    player->setWindowCallback(initWindow);
    player->setTotalTimeCallback(setTotalTime);
    player->ffmpegMusic->setParseCall(parseCall);
    player->ffmpegVideo->setParseCall(parseCall);

    player->init(inputPath);
    player->play();
}


class MemoryTest {

};

//void test() {
//    leaktracer::MemoryTrace::GetInstance().startMonitoringAllThreads();
//    MemoryTest *memoryTest = new MemoryTest;
////    delete memoryTest;
//    leaktracer::MemoryTrace::GetInstance().stopAllMonitoring();
//
//    std::ofstream out;
//    out.open("/sdcard/log.txt", std::ios_base::out);
//    if (out.is_open()) {
//        leaktracer::MemoryTrace::GetInstance().writeLeaks(out);
//    } else {
//        LOGE("Failed to write to \"leaks.out\"\n");
//    }
//}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1play(JNIEnv *env, jobject instance, jstring inputPath_, long nativePlayer) {

    stop(env);

//    leaktracer::MemoryTrace::GetInstance().startMonitoringAllThreads();

    if (pJavaVM == NULL) {
        env->GetJavaVM(&pJavaVM);
    }

    if (pInstance == NULL) {
        pInstance = env->NewGlobalRef(instance);
    }

    inputPath = const_cast<char *>(env->GetStringUTFChars(inputPath_, 0));

    pthread_create(&p_tid, NULL, begin, NULL);//开启begin线程

    env->ReleaseStringUTFChars(inputPath_, inputPath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1display(JNIEnv *env, jobject instance, jobject surface) {

    //得到界面
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    initWindow();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1stop(JNIEnv *env, jobject instance) {
    LOGE("click stop");
    stop(env);

}


extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1pause(JNIEnv *env, jobject instance) {
//    LOGE("leaks click stop");
//    leaktracer::MemoryTrace::GetInstance().stopAllMonitoring();
//
//    std::ofstream out;
//    out.open("/sdcard/log.txt", std::ios_base::out);
//
//    if (out.is_open()) {
//        LOGE("leaks save");
//        leaktracer::MemoryTrace::GetInstance().writeLeaks(out);
//    } else {
//        LOGE("Failed to write to \"leaks.out\"\n");
//    }

    if (player) {
        player->pause();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1seekTo(JNIEnv *env, jobject instance, jint msec) {
    if (player) {
        player->seekTo(msec / 1000);
        preClock = msec / 1000;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1silence(JNIEnv *env, jobject instance) {
    if (player) {
        player->silence();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1rate(JNIEnv *env, jobject instance, jfloat rate) {
    LOGE("play music->rate 2 =%f", rate)
    if (player) {
        player->setRate(rate);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1cut(JNIEnv *env, jobject instance) {
    if (player) {
        player->cutImage();
    }
}


JNIEXPORT jstring JNICALL
Java_com_ffmpeg_Play__1configuration(JNIEnv *env, jobject instance) {

    // TODO
    char info[10000] = {0};
//    av_register_all();

    sprintf(info, "%s\n", avcodec_configuration());

    return env->NewStringUTF(info);
}

JNIEXPORT jlong JNICALL
Java_com_ffmpeg_Play__1init(JNIEnv *env, jobject instance) {

}