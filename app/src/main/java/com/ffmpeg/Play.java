package com.ffmpeg;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


/**
 * Created by LC on 2017/11/20.
 */

public class Play implements SurfaceHolder.Callback {
    static boolean isload = false;

    static {
        if (!isload) {
            System.loadLibrary("avcodec");
            System.loadLibrary("avdevice");
            System.loadLibrary("avfilter");
            System.loadLibrary("avformat");
            System.loadLibrary("avutil");
            System.loadLibrary("swresample");
            System.loadLibrary("swscale");
            System.loadLibrary("native-lib");
            isload = true;
        }

    }

    private long mNativePlayer;

    private HandlerThread mHandlerThread;
    private Handler mHandler;
    private Handler mMainHandler;

    private SurfaceView surfaceView;

    public Play() {
        mNativePlayer = _init();

        mHandlerThread = new HandlerThread("play");
        mHandlerThread.start();

        mHandler = new Handler(mHandlerThread.getLooper());
        mMainHandler = new Handler(Looper.getMainLooper());
    }

    public void setSurfaceView(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;

        surfaceView.getHolder().addCallback(this);

    }

    public void play(final String path) {
        if (surfaceView == null) {
            return;
        }

        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _play(path);
            }
        });
    }

    public void display(final Surface surface) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _display(surface);
            }
        });
    }

    public void stop() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _stop();
            }
        });
    }

    public void pause() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _pause();
            }
        });
    }

    public void seekTo(final int msec) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _seekTo(msec);
            }
        });
    }

    public void silence() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _silence();
            }
        });
    }

    public void setRate(final float rate) {
        Log.e("yijun", "ratr " + rate);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _rate(rate);
            }
        });
    }

    public void cut() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                _cut();
            }
        });
    }

    public native long _init();

    public native int _play(String path);

    public native void _display(Surface surface);

    public native void _stop();

    public native void _pause();

    public native void _seekTo(int sec);


    public native void _silence();//静音

    public native void _rate(float rate);

    public native void _cut();

    public native String _configuration();

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        display(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height) {
//        display(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    private int mWidth;
    private int mHeight;

    public void changeSize(final int width, final int height) {
        if (width > 0 && height > 0 && width != mWidth && height != mHeight) {
            mWidth = width;
            mHeight = height;
            if (mOnPlayCallback != null) {
                mMainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mOnPlayCallback.onChangeSize(width, height);
                    }
                });
            }
        }
    }

    public void setTotalTime(final int sec) {
        if (mOnPlayCallback != null) {
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnPlayCallback.onTotalTime(sec);
                }
            });
        }
    }

    public void setCurrentTime(final int sec) {
        if (mOnPlayCallback != null) {
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnPlayCallback.onCurrentTime(sec);
                }
            });

        }
    }

    private OnPlayCallback mOnPlayCallback;

    public void setOnPlayCallback(OnPlayCallback onPlayCallback) {
        mOnPlayCallback = onPlayCallback;
    }


    /**
     * 时间单位为毫秒
     */
    public interface OnPlayCallback {
        void onChangeSize(int width, final int height);

        void onTotalTime(int sec);

        void onCurrentTime(int sec);

        void onGetCurrentImage(Bitmap bitmap);
    }

    public void setCurrentImage(int[] resultPixes, int w, int h) {
        for (int i = 0; i < resultPixes.length; i++) {
            int c = resultPixes[i];
//            resultPixes[i] = ((c & 0xffffff00) >> 8) | 0xff000000;
//            resultPixes[i] = c & 0x00ffffff | 0xff000000; //A

//            resultPixes[i] = c & 0xff00ffff| 0x00ff0000; //R

//            resultPixes[i] = c & 0xffff00ff | 0x0000ff00; //G
//
//            resultPixes[i] = c & 0xffffff00 | 0x000000ff; //B

            //视频颜色是ARGB，ARGB_4444的颜色的ABRG，需要交换一下R和B
            int B = c & 0x000000ff;//B
            int R = (c & 0x00ff0000) >> 16;//R
            resultPixes[i] = c & 0xff00ff00 | (B << 16) | R;
        }

        Log.e("yijun", "setCurrentImage" + resultPixes.length + " w=" + w + " h=" + h);
        final Bitmap bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_4444);
        bitmap.setPixels(resultPixes, 0, w, 0, 0, w, h);
        if (mOnPlayCallback != null) {
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnPlayCallback.onGetCurrentImage(bitmap);
                }
            });
        }
    }
}
