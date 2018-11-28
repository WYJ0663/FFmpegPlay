//
// Created by yijunwu on 2018/11/28.
//

#include "gles_draw.h"

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
        "    gl_FragColor = vec4(texture2D(u_TextureUnit, v_TextureCoordinates).rgb, 1);  \n"
        "}                                               	\n";


GLboolean glesInit(GLESContexts *contexts, int width, int height) {
    LOGE("VERTEX_SHADER %s FRAGMENT_SHADER %s", vShaderStr, fShaderStr)
    contexts->program = createProgram(vShaderStr, fShaderStr);
    if (!contexts->program) {
        LOGE("Program fail");
        return GL_FALSE;
    }
    //纹理
    glGenTextures(1, &contexts->mTextureID);
    glBindTexture(GL_TEXTURE_2D, contexts->mTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glEnable(GL_TEXTURE_2D);

    glViewport(0, 0, width, height);
    return GL_TRUE;
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

void glesDraw(GLESContexts *contexts, int width, int height, char *pixel) {
    // Set the viewport

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(contexts->program);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
//    glGenerateMipmap(GL_TEXTURE_2D);
    //GL_UNSIGNED_SHORT_5_6_5

    // Retrieve uniform locations for the shader program.
    GLint uTextureUnitLocation = glGetUniformLocation(contexts->program, "u_TextureUnit");
    setUniforms(uTextureUnitLocation, contexts->mTextureID);

    // Retrieve attribute locations for the shader program.
    GLint aPositionLocation = glGetAttribLocation(contexts->program, "a_Position");
    GLint aTextureCoordinatesLocation = glGetAttribLocation(contexts->program, "a_TextureCoordinates");

    //传入顶点坐标
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, sPos);
    glEnableVertexAttribArray(aPositionLocation);
    //传入纹理坐标
    glVertexAttribPointer(aTextureCoordinatesLocation, 2, GL_FLOAT, GL_FALSE, 0, sCoord);
    glEnableVertexAttribArray(aTextureCoordinatesLocation);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void glesDestroye(int width, int height) {
    glBindTexture(GL_TEXTURE_2D, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}
