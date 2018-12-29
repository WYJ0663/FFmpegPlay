//
// Created by yijunwu on 2018/10/19.
//
#ifndef FF_PLAYER_H
#define FF_PLAYER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/native_window.h>

#include "queue.h"
#include "sonic/sonic.h"

#include "ff_player_def.h"

#define MAX_QUEUE_SIZE 500
#define MIN_QUEUE_SIZE 250


Player *ffp_create_player();
//
void ffp_init_ffmpeg(Player *player, char *url);
int ffp_init_audio_ffmpeg(Audio *audio);

//外部调用
void ffp_play(Player *player);
void ffp_stop(Player *player);
void ffp_free(Player *player);


void seek_to(Player *player);

//
#endif
