//
// Created by yijunwu on 2018/10/22.
//
#include <jni.h>
#include <android/native_window_jni.h>
#include "ff_player_def.h"
#include "log.h"
#include "android_jni.h"

Player *get_player(JNIEnv const *env, const void *instance) {
    jclass objClass = (*env)->GetObjectClass(env, instance);
    jfieldID ageID = (*env)->GetFieldID(env, objClass, "mNativePlayer", "J");
    jlong id = (*env)->GetLongField(env, instance, ageID);

    Player *player = (Player *) id;
    return player;
}


void init_window(Player *player) {
    LOGE("surfaceChanged init_window player %d %d %d %d", player, player->video, player->androidJNI->window,
         player->video->height);
    if (player && player->video && player->androidJNI->window && player->video->width > 0) {
        ANativeWindow_setBuffersGeometry(player->androidJNI->window,
                                         player->video->width,
                                         player->video->height,
                                         WINDOW_FORMAT_RGBA_8888);
        LOGE("surfaceChanged init_window");
    }
}

void change_window_size(Player *player) {
    int width = player->video->codec->width;
    int height = player->video->codec->height;

    JavaVM *pJavaVM = player->androidJNI->pJavaVM;
    jobject pInstance = player->androidJNI->pInstance;

    if (player->androidJNI->isSize || NULL == pJavaVM || NULL == pInstance) {
        return;
    }

    JNIEnv *env;
    (*pJavaVM)->AttachCurrentThread(pJavaVM, &env, NULL);

    static jmethodID methodID;
    if (NULL != env && NULL == methodID) {
        jclass clazz = (*env)->GetObjectClass(env, pInstance);
        methodID = (*env)->GetMethodID(env, clazz, "changeSize", "(II)V");
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (NULL != methodID && width > 0 && height > 0) {
        (*env)->CallVoidMethod(env, pInstance, methodID, width, height);
        LOGE("changeSize %d", width);
        player->androidJNI->isSize = true;
    } else {
        LOGE("methodID == null");
    }

//    (*pJavaVM)->DetachCurrentThread(pJavaVM);
}

void set_total_time(Player *player) {
    int64_t duration = player->duration;
    JavaVM *pJavaVM = player->androidJNI->pJavaVM;
    jobject pInstance = player->androidJNI->pInstance;

    if (NULL == pJavaVM || NULL == pInstance) {
        return;
    }

    JNIEnv *env;
    (*pJavaVM)->AttachCurrentThread(pJavaVM, &env, NULL);

    static jmethodID methodID;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = (*env)->GetObjectClass(env, pInstance);
        methodID = (*env)->GetMethodID(env, clazz, "setTotalTime", "(I)V");
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (NULL != methodID && duration > 0) {
        LOGE("setTotalTime == null%d", (jint) (duration / 1000));
        (*env)->CallVoidMethod(env, pInstance, methodID, (jint) (duration / 1000));
    } else {
        LOGE("methodID == null");
    }
//    (*pJavaVM)->DetachCurrentThread(pJavaVM);
}


void set_current_time(Audio *audio, double duration) {
    if ((duration - audio->androidJNI->preClock) < 1) {
        return;
    }
    audio->androidJNI->preClock = duration;

    JavaVM *pJavaVM = audio->androidJNI->pJavaVM;
    jobject pInstance = audio->androidJNI->pInstance;

    if (NULL == pJavaVM || NULL == pInstance) {
        return;
    }

    JNIEnv *env;
    (*pJavaVM)->AttachCurrentThread(pJavaVM, &env, NULL);

    static jmethodID methodID = NULL;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = (*env)->GetObjectClass(env, pInstance);
        methodID = (*env)->GetMethodID(env, clazz, "setCurrentTime", "(I)V");
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (NULL != methodID && duration > 0) {
        (*env)->CallVoidMethod(env, pInstance, methodID, (jint) (duration * 1000));
    } else {
        LOGE("methodID == null");
    }
//    (*pJavaVM)->DetachCurrentThread(pJavaVM);
}

void set_current_image(Player *player, AVFrame *frame) {
    if (!player) {
        return;
    }

    JavaVM *pJavaVM = player->androidJNI->pJavaVM;
    jobject pInstance = player->androidJNI->pInstance;

    if (NULL == pJavaVM || NULL == pInstance) {
        return;
    }

    if (!player->isCutImage) {
        return;
    }

    player->isCutImage = false;

    LOGE("setCurrentImage isCutImage=%d", player->isCutImage);
    JNIEnv *env;
    (*pJavaVM)->AttachCurrentThread(pJavaVM, &env, NULL);

    int w = player->video->codec->width;
    int h = player->video->codec->height;
    uint8_t *src = frame->data[0];
    int size = w * h;
//    uint8_t *drec = new uint8_t[size * 4];
//    memcpy(drec, src, size * 4);
    jintArray result = (*env)->NewIntArray(env, size);
    (*env)->SetIntArrayRegion(env, result, 0, size, (const jint *) src);

    static jmethodID methodID = NULL;
    if (NULL != env && NULL != pInstance && NULL == methodID) {
        jclass clazz = (*env)->GetObjectClass(env, pInstance);
        methodID = (*env)->GetMethodID(env, clazz, "setCurrentImage", "([III)V");
        (*env)->DeleteLocalRef(env, clazz);
    }
    LOGE("setCurrentImage w=%d", w);
    if (NULL != methodID) {
        LOGE("setCurrentImage CallVoidMethod");
        (*env)->CallVoidMethod(env, pInstance, methodID, result, w, h);
    } else {
        LOGE("setCurrentImage == null");
    }
//    env->ReleaseIntArrayElements(result, reinterpret_cast<jint *>(src), 0);
//    (*pJavaVM)->DetachCurrentThread(pJavaVM);
}

void cut_image(Player *player) {
    if (!player) {
        return;
    }

    ANativeWindow *window = player->androidJNI->window;
    int32_t format = ANativeWindow_getFormat(window);
}

void call_video_play(Player *player, AVFrame *frame) {
    ANativeWindow *window = player->androidJNI->window;

    if (!window) {
        return;
    }
    set_current_image(player, frame);
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
        LOGE("call_video_play i=%d", i)
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}


