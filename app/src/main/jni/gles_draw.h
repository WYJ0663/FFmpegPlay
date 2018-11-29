//
// Created by yijunwu on 2018/11/28.
//

#ifndef FF_PLAYER_GLES_DRAW_H
#define FF_PLAYER_GLES_DRAW_H

#include <GLES2/gl2.h>
#include <GLES2/gl2.h>
#include <libavutil/frame.h>
#include "gles_draw.h"
#include "log.h"
#include "gles2/gles.h"

typedef struct GLESContexts {

    GLuint program;
    unsigned int yTexture;
    unsigned int uTexture;
    unsigned int vTexture;
    //yuv数据
    char *yBuffer;
    long yBufferSize;
    char *uBuffer;
    long uBufferSize;
    char *vBuffer;
    long vBufferSize;
} GLESContexts;

GLboolean glesInit(GLESContexts *contexts, int width, int height);

void glesDraw(GLESContexts *contexts, int width, int height, uint8_t *yData, uint8_t *uData, uint8_t *vData,
              AVFrame *avFrame);

void glesDestroye(GLESContexts *contexts, int width, int height);

#endif //FF_PLAYER_GLES_DRAW_H
