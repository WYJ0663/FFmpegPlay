#include <jni.h>
#include <string>
#include <MemoryTrace.hpp>
#include <fstream>
#include <android/log.h>
#include "log.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_MemoryTest_start(JNIEnv *env, jclass type) {

    leaktracer::MemoryTrace::GetInstance().startMonitoringAllThreads();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_MemoryTest_stop(JNIEnv *env, jclass type) {

    leaktracer::MemoryTrace::GetInstance().stopAllMonitoring();
    std::ofstream out;
    out.open("/sdcard/log.txt", std::ios_base::out);
    if (out.is_open()) {
        leaktracer::MemoryTrace::GetInstance().writeLeaks(out);
    } else {
        LOGE("Failed to write to \"leaks.out\"\n");
    }

}