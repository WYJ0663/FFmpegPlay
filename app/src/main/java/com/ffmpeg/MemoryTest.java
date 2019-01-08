package com.ffmpeg;

public class MemoryTest {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void start();

    public static native void stop();
}
