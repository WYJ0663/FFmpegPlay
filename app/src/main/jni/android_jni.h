//
// Created by yijunwu on 2018/10/22.
//

#ifndef FF_ANDROID_JNI_H
#define FF_ANDROID_JNI_H

#include "ff_player_def.h"

Player *get_player(JNIEnv const *env, const void *instance);

void init_window(Player *player);

void init_window2(Player *player,int32_t format);

void change_window_size(Player *player);

void set_total_time(Player *player);

void set_current_time(Audio *audio, double duration);

void set_current_image(Player *player, AVFrame *frame);

void call_video_play(Player *player, AVFrame *frame);

#endif