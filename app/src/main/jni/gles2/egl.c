//
// Created by yijunwu on 2018/11/22.
//

#include "egl.h"


//Display → Config → Surface
EGLBoolean eglOpen(EGLContexts *eglContexts) {
    //get
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failure.");
        return EGL_FALSE;
    }
    const char *info = eglQueryString(eglDisplay, EGL_VENDOR);
    LOGE(" eglGetDisplay info %s", info);

    eglContexts->eglDisplay = eglDisplay;
    LOGE(" eglGetDisplay ok");

    //init
    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean success = eglInitialize(eglDisplay, &majorVersion,
                                       &minorVersion);
    if (!success) {
        LOGE(" eglInitialize failure.");
        return EGL_FALSE;
    }
    LOGE(" eglInitialize ok");

    //config
    EGLint numConfigs;
    EGLConfig config;
    static const EGLint CONFIG_ATTRIBS[] = {EGL_BUFFER_SIZE, EGL_DONT_CARE,
                                            EGL_RED_SIZE, 8,
                                            EGL_GREEN_SIZE, 8,
                                            EGL_BLUE_SIZE, 8,
                                            EGL_DEPTH_SIZE, 16,
                                            EGL_ALPHA_SIZE, EGL_DONT_CARE,
                                            EGL_STENCIL_SIZE, EGL_DONT_CARE,
                                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE // the end
    };

    static const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };

    success = eglChooseConfig(eglDisplay, CONFIG_ATTRIBS, &config, 1,
                              &numConfigs);
    if (!success) {
        LOGE(" eglChooseConfig failure.");
        return EGL_FALSE;
    }
    eglContexts->config = config;
    LOGE(" eglChooseConfig ok");

    //create context
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext elgContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, attribs);
    if (elgContext == EGL_NO_CONTEXT) {
        LOGE(" eglCreateContext failure, error is %d", eglGetError());
        return EGL_FALSE;
    }
    eglContexts->eglContext = elgContext;
    LOGE(" eglCreateContext ok");

    //config
    EGLint eglFormat;
    success = eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID,
                                 &eglFormat);
    if (!success) {
        LOGE(" eglGetConfigAttrib failure.");
        return EGL_FALSE;
    }
    eglContexts->eglFormat = eglFormat;
    LOGE(" eglGetConfigAttrib ok");


    return EGL_TRUE;
}

EGLBoolean eglLinkWindow(EGLContexts *eglContexts, ANativeWindow *window) {

    EGLSurface eglSurface = eglCreateWindowSurface(eglContexts->eglDisplay, eglContexts->config, window, 0);
    if (NULL == eglSurface) {
        LOGE(" eglCreateWindowSurface failure.");
        return EGL_FALSE;
    }
    eglContexts->eglSurface = eglSurface;
    LOGE(" eglCreateWindowSurface ok");

    //关联屏幕
    EGLBoolean is = eglMakeCurrent(eglContexts->eglDisplay, eglSurface, eglSurface, eglContexts->eglContext);
    if (!is) {
        return EGL_FALSE;
    }
    LOGE(" eglMakeCurrent ok");
    return EGL_TRUE;
}

void eglDisplay(EGLContexts *eglContexts) {
    EGLBoolean res = eglSwapBuffers(eglContexts->eglDisplay, eglContexts->eglSurface);
    if (res == EGL_FALSE) {
        LOGE("eglSwapBuffers Error %d", eglGetError());
    } else {
        LOGE("eglSwapBuffers Ok");
    }
}

int eglDestroye(EGLContexts *eglContexts) {
    EGLBoolean success = eglDestroySurface(eglContexts->eglDisplay,
                                           eglContexts->eglSurface);
    if (!success) {
        LOGE("eglDestroySurface failure.");
    }

    success = eglDestroyContext(eglContexts->eglDisplay,
                                eglContexts->eglContext);
    if (!success) {
        LOGE("eglDestroySurface failure.");
    }

    success = eglTerminate(eglContexts->eglDisplay);
    if (!success) {
        LOGE("eglDestroySurface failure.");
    }

    eglContexts->eglSurface = NULL;
    eglContexts->eglContext = NULL;
    eglContexts->eglDisplay = NULL;

    return 0;
}
