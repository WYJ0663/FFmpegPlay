package com.ffmpeg;

import android.os.Build;
import android.os.Environment;
import android.support.annotation.RequiresApi;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.VideoView;
import com.example.ffmpeg.R;

import java.io.File;

public class PlayActivity extends AppCompatActivity implements SurfaceHolder.Callback {


    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("ffmpeg");
        System.loadLibrary("native-lib");
    }

        public static final String url = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
//    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test.mp4";
//    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test1.h264";
//    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test.yuv";

    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_play);

        File file = new File(url);
        Log.e("yijun", "file.exists()" + file.exists());

        final TextView tv = (TextView) findViewById(R.id.tv);
        mSurfaceView = (SurfaceView) findViewById(R.id.surface_view);
        tv.setMovementMethod(ScrollingMovementMethod.getInstance());

        mSurfaceHolder = mSurfaceView.getHolder();
        mSurfaceHolder.addCallback(this);

        findViewById(R.id.play).setOnClickListener(new View.OnClickListener() {
            @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
            @Override
            public void onClick(View v) {
                play(url, mSurfaceHolder.getSurface());
            }
        });

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {

            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public static native void play(String url, Surface surface);

}
