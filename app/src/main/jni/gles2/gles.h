//
// Created by yijunwu on 2018/11/22.
//

#ifndef FF_PLAYER_GLES_H
#define FF_PLAYER_GLES_H

#include <GLES2/gl2.h>
#include <malloc.h>

#include "../log.h"

GLuint createProgram(const char *vtxSrc, const char *fragSrc);

#endif //FF_PLAYER_GLES_H
