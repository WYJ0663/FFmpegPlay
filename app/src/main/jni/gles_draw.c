//
// Created by yijunwu on 2018/11/28.
//

#include <memory.h>
#include "gles_draw.h"
#include "gles_draw_shader.h"

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

    //为yuv数据分配存储空间
    contexts->yBufferSize = sizeof(char) * width * height;
    contexts->uBufferSize = sizeof(char) * width * height / 4;
    contexts->vBufferSize = sizeof(char) * width * height / 4;
    contexts->yBuffer = (char *) malloc(contexts->yBufferSize);
    contexts->uBuffer = (char *) malloc(contexts->uBufferSize);
    contexts->vBuffer = (char *) malloc(contexts->vBufferSize);
    memset(contexts->yBuffer, 0, contexts->yBufferSize);
    memset(contexts->uBuffer, 0, contexts->uBufferSize);
    memset(contexts->vBuffer, 0, contexts->vBufferSize);

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


void glesDraw(GLESContexts *contexts, int width, int height, uint8_t *yData, uint8_t *uData, uint8_t *vData) {
    //set data
    memcpy(contexts->yBuffer, yData, contexts->yBufferSize);
    memcpy(contexts->uBuffer, uData, contexts->uBufferSize);
    memcpy(contexts->vBuffer, vData, contexts->vBufferSize);

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

    glDeleteProgram(contexts->program);
    glDeleteTextures(1, &contexts->yTexture);
    glDeleteTextures(1, &contexts->uTexture);
    glDeleteTextures(1, &contexts->vTexture);
    free(contexts->yBuffer);
    free(contexts->uBuffer);
    free(contexts->vBuffer);
}
