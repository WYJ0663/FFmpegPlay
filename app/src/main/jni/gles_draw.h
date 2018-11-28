//
// Created by yijunwu on 2018/11/28.
//

#ifndef FF_PLAYER_GLES_DRAW_H
#define FF_PLAYER_GLES_DRAW_H

#include <GLES2/gl2.h>
#include <GLES2/gl2.h>
#include "gles_draw.h"
#include "log.h"
#include "gles2/gles.h"
typedef struct GLESContexts {

    GLuint program;
    GLuint mTextureID;

} GLESContexts;

GLboolean glesInit(GLESContexts *contexts, int width, int height);

void glesDraw(GLESContexts *contexts, int width, int height, char *pixel);

void glesDestroye(int width, int height);

#endif //FF_PLAYER_GLES_DRAW_H
