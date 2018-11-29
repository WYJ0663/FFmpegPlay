//
// Created by yijunwu on 2018/11/28.
//

#ifndef FF_PLAYER_GLES_DRAW_SHADER_H
#define FF_PLAYER_GLES_DRAW_SHADER_H

#include <GLES2/gl2.h>

#define GLES_STRINGIZE(x)   #x

GLbyte vShaderStr[] = GLES_STRINGIZE(
        attribute vec3 aPosition;
        attribute vec2 aTexCoor;
        varying vec2 vTexCoor;
        void main()
        {
            gl_Position = vec4(aPosition, 1);
            vTexCoor = aTexCoor;
        }
);

GLbyte fShaderStr[] =GLES_STRINGIZE(
        precision mediump float;
        uniform sampler2D yTexture;
        uniform sampler2D uTexture;
        uniform sampler2D vTexture;
        varying vec2 vTexCoor;
        void main()
        {
            float y = texture2D(yTexture, vTexCoor).r;
            float u = texture2D(uTexture, vTexCoor).r;
            float v = texture2D(vTexture, vTexCoor).r;
            vec3 yuv = vec3(y, u, v);
            vec3 offset = vec3(16.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0);
            mat3 mtr = mat3(1.0, 1.0, 1.0, -0.001, -0.3441, 1.772, 1.402, -0.7141, 0.001);
            vec4 curColor = vec4(mtr * (yuv - offset), 1);
            gl_FragColor = curColor;
        }
);


#endif //FF_PLAYER_GLES_DRAW_SHADER_H
