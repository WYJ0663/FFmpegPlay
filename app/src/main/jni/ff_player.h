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

#include "ff_packet_queuel.h"
#include "sonic/sonic.h"

#include "ff_player_def.h"


Player *ffp_create_player();
void ffp_init_player(Player *player);//一次性数据
void ffp_init_audio(Audio *audio);
void ffp_init_video(Video *video);
void ffp_destroy_player(Player *player);
void ffp_destroy_audio(Audio *audio);
void ffp_destroy_video(Video *video);

//
void ffp_reset_player(Player *player);//参数设置
void ffp_reset_audio(Audio *audio);
void ffp_reset_video(Video *video);

//
void ffp_init_ffmpeg(Player *player, char *url);
int ffp_init_audio_ffmpeg(Audio *audio);

//外部调用
void ffp_play(Player *player);
void ffp_stop(Player *player);

//
#endif
