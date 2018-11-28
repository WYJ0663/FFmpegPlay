#ifndef FF_PLAYER_EGL_H
#define FF_PLAYER_EGL_H

#include <EGL/egl.h>
#include "../log.h"
#include <android/native_window_jni.h>

typedef struct EGLContexts {
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLContext eglContext;
    EGLint eglFormat;//颜色格式

    EGLConfig config;

} EGLContexts;

//1、初始化
EGLBoolean eglOpen(EGLContexts *eglContexts);

//2、关联window
EGLBoolean eglLinkWindow(EGLContexts *eglContexts, ANativeWindow *window);

//3、初始化EGL和显示需要再通一个线程
void eglDisplay(EGLContexts *eglContexts);

//4、销毁
int eglDestroye(EGLContexts *eglContexts);

#endif //FF_PLAYER_GLES_H
