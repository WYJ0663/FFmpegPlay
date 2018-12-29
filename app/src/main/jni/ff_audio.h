//
// Created by yijunwu on 2018/10/19.
//
#ifndef FF_AUDIO_H
#define FF_AUDIO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/native_window.h>

#include "queue.h"
#include "ff_player.h"
#include "ff_player_def.h"

bool create_openelse(Audio *audio);

void put_audio_packet(Audio *audio, AVPacket *avPacket);

void *ffp_start_audio_play (void *arg);

void ffp_audio_free(Audio *audio);

#endif

