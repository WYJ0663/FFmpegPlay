package com.ffmpeg;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import com.example.ffmpeg.R;

import java.io.File;
import java.text.SimpleDateFormat;

public class MainActivity extends AppCompatActivity implements Play.OnPlayCallback {

    private Play mPlayer;
    private SurfaceView surfaceView;
    private TextView mTextView, mTextCurTime;
    private SeekBar mSeekBar;

    private SeekBar mRateBar;
    private TextView mRateView;

    private ImageView mImageView;

    private TextView mInfoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();

        mPlayer = new Play();
        mPlayer.setOnPlayCallback(this);
        mPlayer.setSurfaceView(surfaceView);
//        mInfoView.setText(mPlayer._configuration());
    }

    private void initView() {
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        mTextView = (TextView) findViewById(R.id.textview);
        mSeekBar = (SeekBar) findViewById(R.id.seekBar);
        mTextCurTime = (TextView) findViewById(R.id.tvcur);

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                mPlayer.seekTo(seekBar.getProgress());
            }
        });

        mRateBar = (SeekBar) findViewById(R.id.rate_bar);
        mRateView = (TextView) findViewById(R.id.rate_text);
        mRateBar.setMax(7);
        mRateBar.setProgress(3);
        mRateView.setText("1.0X");

        mRateBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                int i = seekBar.getProgress();
                float rate = 0.25f * (i + 1);
                mPlayer.setRate(rate);
                mRateView.setText(rate + "X");
            }
        });

        mImageView = (ImageView) findViewById(R.id.image);
        mInfoView = (TextView) findViewById(R.id.info);
    }

    //    public static final String url = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test3.mp4";
//    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test1.h264";
//    public static final String url = Environment.getExternalStorageDirectory().getPath() + "/test.yuv";

    public void player(View view) {
        mPlayer.play(url);
    }

    public static final String url2 = Environment.getExternalStorageDirectory().getPath() + "/test2.mp4";

    public void player2(View view) {
        mPlayer.play(url2);
    }

    public static final String url3 = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";

    public void player3(View view) {
        mPlayer.play(url3);
    }

    public static final String url4 = Environment.getExternalStorageDirectory().getPath() + "/test4.mp4";

    public void player4(View view) {
        mPlayer.play(url4);
    }

    public void stop(View view) {
        mPlayer.stop();
    }

    public void pause(View view) {
        mPlayer.pause();
    }

    public void silence(View view) {
        mPlayer.silence();
    }

    @Override
    public void onChangeSize(final int width, final int height) {
        if (surfaceView != null && surfaceView.getLayoutParams() != null) {
            surfaceView.post(new Runnable() {
                @Override
                public void run() {
                    int w = SystemUtil.getDisplayWidth(surfaceView.getContext());
                    int h = w * height / width;
                    Log.e("yijun", "h " + h);
                    surfaceView.getLayoutParams().width = w;
                    surfaceView.getLayoutParams().height = h;
                    surfaceView.requestLayout();
                }
            });
        }
    }

    @Override
    public void onTotalTime(int sec) {
        Log.e("yijun", "onTotalTime");
        mSeekBar.setMax(sec);
        mTextView.setText(formatTime(sec));
        onCurrentTime(0);
    }

    @Override
    public void onCurrentTime(int sec) {
        Log.e("yijun", "onCurrentTime" + sec);
        mSeekBar.setProgress(sec);
        mTextCurTime.setText(formatTime(sec));
    }

    Bitmap mBitmap;

    @Override
    public void onGetCurrentImage(Bitmap bitmap) {
        mImageView.setImageBitmap(bitmap);
        if (mBitmap != null && mBitmap.isRecycled()) {
            mBitmap.recycle();
            mBitmap = null;
        }
        mBitmap = bitmap;
    }

    private String formatTime(long time) {
        SimpleDateFormat format = new SimpleDateFormat("mm:ss");
        return format.format(time);
    }

    public void cut(View view) {
        mPlayer.cut();
    }

}
