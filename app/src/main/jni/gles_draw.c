//
// Created by yijunwu on 2018/11/28.
//

#include <memory.h>
#include "gles_draw.h"

GLbyte vShaderStr[] = "attribute vec3 aPosition;							\n" \
"attribute vec2 aTexCoor; 							\n" \
"varying vec2 vTexCoor;		 						\n" \
"void main() 										\n" \
"{ 													\n" \
"	gl_Position = vec4(aPosition, 1); 	\n" \
" 	vTexCoor = aTexCoor;							\n" \
"} 													\n" \
;

GLbyte fShaderStr[] = "precision mediump float;											\n" \
"uniform sampler2D yTexture; 										\n" \
"uniform sampler2D uTexture; 										\n" \
"uniform sampler2D vTexture; 										\n" \
"varying vec2 vTexCoor;												\n" \
"void main()														\n" \
"{																	\n" \
"	float y = texture2D(yTexture, vTexCoor).r;						\n" \
"	float u = texture2D(uTexture, vTexCoor).r;											\n" \
"	float v = texture2D(vTexture, vTexCoor).r;													\n" \
"	vec3 yuv = vec3(y, u, v);												\n" \
"	vec3 offset = vec3(16.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0);								\n" \
"	mat3 mtr = mat3(1.0, 1.0, 1.0, -0.001, -0.3441, 1.772, 1.402, -0.7141, 0.001);						\n" \
"	vec4 curColor = vec4(mtr * (yuv - offset), 1);												\n" \
"	gl_FragColor = curColor;					\n" \
"}																	\n" \
;

//
//GLboolean checkGlError(const char *funcName) {
//    GLint err = glGetError();
//    LOGE("GLES error after %s(): 0x%08x\n", funcName, err);
//    if (err != GL_NO_ERROR) {
//        LOGE("GL true");
//        return GL_TRUE;
//    }
//    LOGE("GL false");
//    return GL_FALSE;
//}

GLboolean glesInit(GLESContexts *contexts, int width, int height) {
    LOGE("glesInit VERTEX_SHADER %s FRAGMENT_SHADER %s", vShaderStr, fShaderStr)
    contexts->program = createProgram(vShaderStr, fShaderStr);
    if (!contexts->program) {
        LOGE("glesInit Program fail");
        return GL_FALSE;
    }
    //纹理
    glGenTextures(1, &contexts->yTexture);
    glGenTextures(1, &contexts->uTexture);
    glGenTextures(1, &contexts->vTexture);
//    glEnable(GL_TEXTURE_2D);

    //为yuv数据分配存储空间
    contexts->yBufferSize = sizeof(char) * width * height;
    contexts->uBufferSize = sizeof(char) * width * height / 4;
    contexts->vBufferSize = sizeof(char) * width * height / 4;
    LOGE("video->codec %d,%d,%d",  contexts->yBufferSize, contexts->uBufferSize, contexts->vBufferSize);
    contexts->yBuffer = (char *) av_malloc(contexts->yBufferSize);
    contexts->uBuffer = (char *) av_malloc(contexts->uBufferSize);
    contexts->vBuffer = (char *) av_malloc(contexts->vBufferSize);
//    memset(contexts->yBuffer, 0, contexts->yBufferSize);
//    memset(contexts->uBuffer, 0, contexts->uBufferSize);
//    memset(contexts->vBuffer, 0, contexts->vBufferSize);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width, height);
    return GL_TRUE;
}

void setUniforms(int uTextureUnitLocation, GLenum texture_n, int textureId, int width, int height, const void *buffer) {
    // Pass the matrix into the shader program.
    //glUniformMatrix4fv(uMatrixLocation, 1, false, matrix);

    // Set the active texture unit to texture unit 0.
    glActiveTexture(texture_n);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
    // Tell the texture uniform sampler to use this texture in the shader by
    // telling it to read from texture unit 0.
    switch (texture_n) {
        case GL_TEXTURE0:
            glUniform1i(uTextureUnitLocation, 0);
            break;
        case GL_TEXTURE1:
            glUniform1i(uTextureUnitLocation, 1);
            break;
        case GL_TEXTURE2:
            glUniform1i(uTextureUnitLocation, 2);
            break;
    }

}

const GLfloat sPos[] = {
        -1.0f, 1.0f,    //左上角
        -1.0f, -1.0f,   //左下角
        1.0f, 1.0f,     //右上角
        1.0f, -1.0f     //右下角
};

const GLfloat sCoord[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
};

void copyFrameData(char *dst, uint8_t *src, int width, int height, int linesize) {
    for (int i = 0; i < height; ++i) {
        memcpy(dst, src, width);
        dst += width;
        src += linesize;
    }
}

void glesDraw(GLESContexts *contexts, int width, int height, uint8_t *yData, uint8_t *uData, uint8_t *vData,
              AVFrame *avFrame) {
    //set data
    memcpy(contexts->yBuffer, yData, contexts->yBufferSize);
    memcpy(contexts->uBuffer, uData, contexts->uBufferSize);
    memcpy(contexts->vBuffer, vData, contexts->vBufferSize);

//    copyFrameData(contexts->yBuffer, avFrame->data[0], width, height, avFrame->linesize[0]);
//    copyFrameData(contexts->uBuffer, avFrame->data[1], width/2, height/2, avFrame->linesize[1]);
//    copyFrameData(contexts->vBuffer, avFrame->data[2], width/2, height/2, avFrame->linesize[2]);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(contexts->program);

    //获取采样器索引
    GLint myTextureHandle = glGetUniformLocation(contexts->program, "yTexture");
    GLint muTextureHandle = glGetUniformLocation(contexts->program, "uTexture");
    GLint mvTextureHandle = glGetUniformLocation(contexts->program, "vTexture");

    // Retrieve uniform locations for the shader program.
    setUniforms(myTextureHandle, GL_TEXTURE0, contexts->yTexture, width, height, contexts->yBuffer);
    setUniforms(muTextureHandle, GL_TEXTURE1, contexts->uTexture, width / 2, height / 2, contexts->uBuffer);
    setUniforms(mvTextureHandle, GL_TEXTURE2, contexts->vTexture, width / 2, height / 2, contexts->vBuffer);

//    checkGlError("GL_TEXTURE");

    // Retrieve attribute locations for the shader program.
    GLint aPositionLocation = glGetAttribLocation(contexts->program, "aPosition");
    GLint aTextureCoordinatesLocation = glGetAttribLocation(contexts->program, "aTexCoor");

    //传入顶点坐标
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, sPos);
    glEnableVertexAttribArray(aPositionLocation);
    //传入纹理坐标
    glVertexAttribPointer(aTextureCoordinatesLocation, 2, GL_FLOAT, GL_FALSE, 0, sCoord);
    glEnableVertexAttribArray(aTextureCoordinatesLocation);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void glesDestroye(GLESContexts *contexts, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    av_free(contexts->yBuffer);
    av_free(contexts->uBuffer);
    av_free(contexts->vBuffer);
}
