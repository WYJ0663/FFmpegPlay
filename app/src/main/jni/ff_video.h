//
// Created by yijunwu on 2018/10/22.
//
//
#ifndef FF_VIDEO_H
#define FF_VIDEO_H

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <pthread.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include "ff_player_def.h"
#include "log.h"

void put_video_packet(Video *video, AVPacket *avPacket);

void *ffp_start_video_play(void *args);

#endif