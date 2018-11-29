
#ifndef FFMPEGMUSIC_LOG_H
#define FFMPEGMUSIC_LOG_H

#include <android/log.h>

#define TRUE  1
#define FALSE 0

#define DEBUG_MODE TRUE

#if DEBUG_MODE
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"qiqi",FORMAT,##__VA_ARGS__);
#else
#define  LOGE(...)
#endif


#endif //FFMPEGMUSIC_LOG_H
