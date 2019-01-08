#include <jni.h>
#include <android/native_window_jni.h>
#include <pthread.h>

#include "ff_player_def.h"
#include "ff_player.h"
#include "android_jni.h"
#include "log.h"

JNIEXPORT jlong JNICALL
Java_com_ffmpeg_Play__1init(JNIEnv *env, jobject instance) {
    Player *player = ffp_create_player();
    (*env)->GetJavaVM(env, &player->androidJNI->pJavaVM);
    player->androidJNI->pInstance = (*env)->NewGlobalRef(env, instance);
    return (jlong) player;
}


JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1play(JNIEnv *env, jobject instance, jstring inputPath_) {

    char *url = (char *) (*env)->GetStringUTFChars(env, inputPath_, 0);

    Player *player = get_player(env, instance);

    ffp_init_ffmpeg(player, url);

    ffp_play(player);

    //内存溢出测试
//    char*s=malloc(1024);
}


JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1display(JNIEnv *env, jobject instance, jobject surface) {

    Player *player = get_player(env, instance);

    if (player->androidJNI->window) {
        ANativeWindow_release(player->androidJNI->window);
        player->androidJNI->window = 0;
    }

    player->androidJNI->window = ANativeWindow_fromSurface(env, surface);
//    init_window(player);
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1stop(JNIEnv *env, jobject instance) {
    Player *player = get_player(env, instance);
    ffp_stop(player);
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1pause(JNIEnv *env, jobject instance) {
    Player *player = get_player(env, instance);
    if (player->status->isPause) {
        player->status->isPause = false;
        pthread_cond_signal(&player->audio->queue->cond);
        pthread_cond_signal(&player->video->queue->cond);
    } else {
        player->status->isPause = true;
    }
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1seekTo(JNIEnv *env, jobject instance, jint msec) {
    Player *player = get_player(env, instance);
    LOGE("1 切进度 %d", msec);
    player->seekTime = msec / 1000;
    seek_to(player);
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1silence(JNIEnv *env, jobject instance) {
    Player *player = get_player(env, instance);
    if (player->audio->isSilence) {
        player->audio->isSilence = false;
    } else {
        player->audio->isSilence = true;
    }
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1rate(JNIEnv *env, jobject instance, jfloat rate) {
    Player *player = get_player(env, instance);
    if (rate > 0) {
        player->audio->rate = rate;
    }
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1cut(JNIEnv *env, jobject instance) {
    Player *player = get_player(env, instance);
    player->isCutImage = true;
}

JNIEXPORT jstring JNICALL
Java_com_ffmpeg_Play__1configuration(JNIEnv *env, jobject instance) {

    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());

    return (*env)->NewStringUTF(env, info);
}
