#include <jni.h>

#include "ff_player_def.h"
#include "ff_player.h"

Player *get_player(JNIEnv const *env, const void *instance);

Player *get_player(JNIEnv const *env, const void *instance) {
    jclass objClass = (*env)->GetObjectClass(env, instance);
    jfieldID ageID = (*env)->GetFieldID(env, objClass, "mNativePlayer", "J");
    jlong id = (*env)->GetLongField(env, instance, ageID);

    Player *player = (Player *) id;
    return player;
}

JNIEXPORT jlong JNICALL
Java_com_ffmpeg_Play__1init(JNIEnv *env, jobject instance) {
    Player *player = ffp_create_player();
    (*env)->GetJavaVM(env, &player->pJavaVM);
    player->pInstance = (*env)->NewGlobalRef(env, instance);
    return (jlong) player;
}


void JNICALL
Java_com_ffmpeg_Play__1play(JNIEnv *env, jobject instance, jstring inputPath_) {

    char *url = (char *) (*env)->GetStringUTFChars(env, inputPath_, 0);

    Player *player = get_player(env, instance);

    player->isPlay = false;

    ffp_init_ffmpeg(player, url);

    ffp_play(player);
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1display(JNIEnv *env, jobject instance, jobject surface) {

}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1stop(JNIEnv *env, jobject instance) {

}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1pause(JNIEnv *env, jobject instance) {
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1seekTo(JNIEnv *env, jobject instance, jint msec) {
}


JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1silence(JNIEnv *env, jobject instance) {
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1rate(JNIEnv *env, jobject instance, jfloat rate) {
}

JNIEXPORT void JNICALL
Java_com_ffmpeg_Play__1cut(JNIEnv *env, jobject instance) {
}

JNIEXPORT jstring JNICALL
Java_com_ffmpeg_Play__1configuration(JNIEnv *env, jobject instance) {

    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());

    return (*env)->NewStringUTF(env, info);
}
