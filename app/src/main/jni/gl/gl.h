#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <jni.h>

#define LOGV(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"LC XXX",FORMAT,##__VA_ARGS__);

typedef struct GlobalContexts {
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLContext eglContext;
    EGLint eglFormat;

    GLuint mTextureID;
    GLuint glProgram;
    GLint positionLoc;

    int width;
    int height;

} GlobalContext;

//1初始化
int CreateProgram();

//2绑定window
void setSurface(JNIEnv *env, jobject obj, jobject surface);

//3设置windows
int32_t setBuffersGeometry(int32_t width, int32_t height);

//4显示
void Render(uint8_t *pixel);

void call_video_play(AVFrame *frame);
