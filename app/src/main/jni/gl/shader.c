#include <GLES2/gl2.h>
#include <malloc.h>
#include <stdbool.h>
#include <android/native_window_jni.h>
#include <libavutil/frame.h>
#include "gl.h"

#define BYTES_PER_FLOAT 4
#define POSITION_COMPONENT_COUNT 2
#define TEXTURE_COORDINATES_COMPONENT_COUNT 2
#define STRIDE ((POSITION_COMPONENT_COUNT + TEXTURE_COORDINATES_COMPONENT_COUNT)*BYTES_PER_FLOAT)

GlobalContext global_context;

ANativeWindow *mANativeWindow;

/* type specifies the Shader type: GL_VERTEX_SHADER or GL_FRAGMENT_SHADER */
GLuint LoadShader(GLenum type, const char *shaderSrc) {
    GLuint shader;
    GLint compiled;

    // Create an empty shader object, which maintain the source code strings that define a shader
    shader = glCreateShader(type);

    if (shader == 0)
        return 0;

    // Replaces the source code in a shader object
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader object
    glCompileShader(shader);

    // Check the shader object compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLen = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            GLchar *infoLog = (GLchar *) malloc(sizeof(GLchar) * infoLen);

            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGV("Error compiling shader:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}


GLuint LoadProgram(const char *vShaderStr, const char *fShaderStr) {
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

    // Create the program object
    programObject = glCreateProgram();
    if (programObject == 0)
        return 0;

    // Attaches a shader object to a program object
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    // Bind vPosition to attribute 0
    glBindAttribLocation(programObject, 0, "vPosition");
    // Link the program object
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint infoLen = 0;

        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            GLchar *infoLog = (GLchar *) malloc(sizeof(GLchar) * infoLen);

            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            LOGV("Error linking program:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteProgram(programObject);
        return GL_FALSE;
    }

    // Free no longer needed shader resources
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programObject;
}


int CreateProgram() {
    GLuint programObject;

    GLbyte vShaderStr[] = "attribute vec4 a_Position;  			\n"
                          "attribute vec2 a_TextureCoordinates;   			\n"
                          "varying vec2 v_TextureCoordinates;     			\n"
                          "void main()                            			\n"
                          "{                                      			\n"
                          "    v_TextureCoordinates = a_TextureCoordinates;   \n"
                          "    gl_Position = a_Position;    					\n"
                          "}                                      			\n";

    GLbyte fShaderStr[] =
            "precision mediump float; \n"
            "uniform sampler2D u_TextureUnit;                	\n"
            "varying vec2 v_TextureCoordinates;              	\n"
            "void main()                                     	\n"
            "{                                               	\n"
            "    gl_FragColor = texture2D(u_TextureUnit, v_TextureCoordinates);  \n"
            "}                                               	\n";

    // Load the shaders and get a linked program object
    programObject = LoadProgram((const char *) vShaderStr,
                                (const char *) fShaderStr);
    if (programObject == 0) {
        return GL_FALSE;
    }

    // Store the program object
    global_context.glProgram = programObject;

    // Get the attribute locations
    global_context.positionLoc = glGetAttribLocation(programObject,
                                                     "v_position");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glGenTextures(1, &global_context.mTextureID);
    glBindTexture(GL_TEXTURE_2D, global_context.mTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_2D);

    return 0;
}

void setUniforms(int uTextureUnitLocation, int textureId) {
    // Pass the matrix into the shader program.
    //glUniformMatrix4fv(uMatrixLocation, 1, false, matrix);

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Tell the texture uniform sampler to use this texture in the shader by
    // telling it to read from texture unit 0.
    glUniform1i(uTextureUnitLocation, 0);
}

void Render(uint8_t *pixel) {
    GLfloat vVertices[] = {0.0f, 0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                           0.5f, -0.5f, 0.0f};

    // Set the viewport
    glViewport(0, 0, global_context.width, global_context.height);

    // Clear the color buffer
    //glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(global_context.glProgram);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, global_context.width,
                 global_context.height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixel);
//GL_UNSIGNED_SHORT_5_6_5

    // Retrieve uniform locations for the shader program.
    GLint uTextureUnitLocation = glGetUniformLocation(global_context.glProgram,
                                                      "u_TextureUnit");
    setUniforms(uTextureUnitLocation, global_context.mTextureID);

    // Retrieve attribute locations for the shader program.
    GLint aPositionLocation = glGetAttribLocation(global_context.glProgram,
                                                  "a_Position");
    GLint aTextureCoordinatesLocation = glGetAttribLocation(
            global_context.glProgram, "a_TextureCoordinates");

    // Order of coordinates: X, Y, S, T
    // Triangle Fan
    GLfloat VERTEX_DATA[] = {0.0f, 0.0f, 0.5f, 0.5f,
                             -1.0f, -1.0f, 0.0f, 1.0f,
                             1.0f, -1.0f, 1.0f, 1.0f,
                             1.0f, 1.0f, 1.0f, 0.0f,
                             -1.0f, 1.0f, 0.0f, 0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f};

    glVertexAttribPointer(aPositionLocation, POSITION_COMPONENT_COUNT, GL_FLOAT,
                          false, STRIDE, VERTEX_DATA);
    glEnableVertexAttribArray(aPositionLocation);

    glVertexAttribPointer(aTextureCoordinatesLocation, POSITION_COMPONENT_COUNT,
                          GL_FLOAT, false, STRIDE, &VERTEX_DATA[POSITION_COMPONENT_COUNT]);
    glEnableVertexAttribArray(aTextureCoordinatesLocation);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    eglSwapBuffers(global_context.eglDisplay, global_context.eglSurface);
}


void renderSurface(uint8_t *pixel) {
//
//	if (false) {
//		glDisable(GL_TEXTURE_2D);
//		glDeleteTextures(1, &global_context.mTextureID);
//		glDeleteProgram(global_context.glProgram);
//		return;
//	}

//	if (global_context.pause) {
//		return;
//	}

    Render(pixel);
}


// format not used now.
int32_t setBuffersGeometry(int32_t width, int32_t height) {
    //int32_t format = WINDOW_FORMAT_RGB_565;
    global_context.width = width;
    global_context.height = height;

    if (NULL == mANativeWindow) {
        LOGV("mANativeWindow is NULL.");
        return -1;
    }

    return ANativeWindow_setBuffersGeometry(mANativeWindow, width, height,
                                            global_context.eglFormat);
}

int eglOpen() {

    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGV("eglGetDisplay failure.");
        return -1;
    }
    global_context.eglDisplay = eglDisplay;
    LOGV(" eglGetDisplay ok");

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean success = eglInitialize(eglDisplay, &majorVersion,
                                       &minorVersion);
    if (!success) {
        LOGV(" eglInitialize failure.");
        return -1;
    }
    LOGV(" eglInitialize ok");

    GLint numConfigs;
    EGLConfig config;
    static const EGLint CONFIG_ATTRIBS[] = {EGL_BUFFER_SIZE, EGL_DONT_CARE,
                                            EGL_RED_SIZE, 5,
                                            EGL_GREEN_SIZE, 6,
                                            EGL_BLUE_SIZE, 5,
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
        LOGV(" eglChooseConfig failure.");
        return -1;
    }
    LOGV(" eglChooseConfig ok");

    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext elgContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT,
                                             attribs);
    if (elgContext == EGL_NO_CONTEXT) {
        LOGV(" eglCreateContext failure, error is %d", eglGetError());
        return -1;
    }
    global_context.eglContext = elgContext;
    LOGV(" eglCreateContext ok");

    EGLint eglFormat;
    success = eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID,
                                 &eglFormat);
    if (!success) {
        LOGV(" eglGetConfigAttrib failure.");
        return -1;
    }
    global_context.eglFormat = eglFormat;
    LOGV(" eglGetConfigAttrib ok");

    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, config,
                                                   mANativeWindow, 0);
    if (NULL == eglSurface) {
        LOGV(" eglCreateWindowSurface failure.");
        return -1;
    }
    global_context.eglSurface = eglSurface;

    LOGV(" eglCreateWindowSurface ok");

    //关联屏幕
    EGLBoolean is = eglMakeCurrent(eglDisplay, eglSurface, eglSurface, elgContext);
    if (is) {
        LOGV(" eglMakeCurrent ok");
    }

    return 0;
}

int eglClose() {
    EGLBoolean success = eglDestroySurface(global_context.eglDisplay,
                                           global_context.eglSurface);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    success = eglDestroyContext(global_context.eglDisplay,
                                global_context.eglContext);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    success = eglTerminate(global_context.eglDisplay);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    global_context.eglSurface = NULL;
    global_context.eglContext = NULL;
    global_context.eglDisplay = NULL;

    return 0;
}


void setSurface(JNIEnv *env, jobject obj, jobject surface) {

    // obtain a native window from a Java surface
    mANativeWindow = ANativeWindow_fromSurface(env, surface);

    LOGV("mANativeWindow ok");
    if ((global_context.eglSurface != NULL)
        || (global_context.eglContext != NULL)
        || (global_context.eglDisplay != NULL)) {
        eglClose();
    }
    eglOpen();
}

void call_video_play(AVFrame *frame) {

    if (!mANativeWindow) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    int re = ANativeWindow_lock(mANativeWindow, &window_buffer, 0);
    if (re) {
        LOGV("绘制 %d", re);
        return;
    }

    LOGV("绘制 宽%d,高%d", frame->width, frame->height);
    LOGV("绘制 宽%d,高%d  行字节 %d ", window_buffer.width, window_buffer.height, frame->linesize[0]);

    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(mANativeWindow);
}
