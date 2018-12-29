//
// Created by yijunwu on 2018/10/19.
//
#include <pthread.h>
#include "ff_audio.h"
#include "log.h"
#include "android_jni.h"


void ffp_audio_free(Audio *audio) {
    if (audio->out_buffer) {
        free(audio->out_buffer);
    }
    if (audio->out_rate_buffer) {
        free(audio->out_rate_buffer);
    }

    if (audio->bqPlayerPlay) {
        (*audio->bqPlayerPlay)->SetPlayState(audio->bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        audio->bqPlayerPlay = 0;
    }
    if (audio->bqPlayerObject) {
        (*audio->bqPlayerObject)->Destroy(audio->bqPlayerObject);
        audio->bqPlayerObject = 0;

        audio->bqPlayerBufferQueue = 0;
        audio->bqPlayerVolume = 0;
    }

    if (audio->outputMixObject) {
        (*audio->outputMixObject)->Destroy(audio->outputMixObject);
        audio->outputMixObject = 0;
    }

    if (audio->engineObject) {
        (*audio->engineObject)->Destroy(audio->engineObject);
        audio->engineObject = 0;
        audio->engineEngine = 0;
    }
    if (audio->swrContext) {
        swr_free(&audio->swrContext);
    }
    if (audio->sonic) {
        sonicDestroyStream(audio->sonic);
    }
}

void set_silence_frame(AVFrame *frame) {
    av_samples_set_silence(frame->data, 0, frame->nb_samples, frame->channels, (enum AVSampleFormat) frame->format);
}

// 设置流的速率
int getSonicData(Audio *audio, int nb) {
    //参数为采样率和声道数
    sonicSetSpeed(audio->sonic, 1);
    sonicSetRate(audio->sonic, audio->rate);
    LOGE("music->rate=%f", audio->rate)

    // 向流中写入pcm_buffer
    int ret = sonicWriteShortToStream(audio->sonic, (short *) (audio->out_buffer), nb);
    // 计算处理后的点数
    int new_buffer_size = 0;
    if (ret) {
        // 从流中读取处理好的数据
        new_buffer_size = sonicReadShortFromStream(audio->sonic, audio->out_rate_buffer, 44100 * 2);
        return new_buffer_size;
    }
    return 0;
}

AVPacket *get_audio_packet(Audio *audio) {

    if (audio->status->isPause) {
        pthread_mutex_lock(&audio->queue->mutex);
        pthread_cond_wait(&audio->queue->cond, &audio->queue->mutex);
        pthread_mutex_unlock(&audio->queue->mutex);
    }

    AVPacket *avPacket = getQueue(audio->queue);

    if (audio->queue->size < MIN_QUEUE_SIZE) {
        pthread_cond_signal(&audio->status->cond);
    }
    return avPacket;
}

void put_audio_packet(Audio *audio, AVPacket *avPacket) {
    LOGE("包  %d ", audio->queue->size);
    AVPacket *dst = av_packet_alloc();

    if (av_packet_ref(dst, avPacket)) {
        av_packet_unref(dst);
        av_packet_free(&dst);
        LOGE("克隆失败")
    }
    if (putQueue(audio->queue, dst) == 0) {
        av_packet_unref(dst);
        av_packet_free(&dst);
        LOGE("插入失败")
    }
}

//得到pcm数据
int get_pcm(Audio *audio) {
//    AVPacket *avPacket = audio->avPacket;
//    AVFrame *avFrame = audio->avFrame;
    int size;
    LOGE("准备解码");
    while (audio->status->isPlay) {
        size = 0;
        AVPacket *avPacket = get_audio_packet(audio);
        //时间矫正
        if (avPacket->pts != AV_NOPTS_VALUE) {
            audio->clock = av_q2d(audio->time_base) * avPacket->pts;
        }
        LOGE("解码");
        if (audio->codec == NULL) {
            LOGE("没有解码器");
        }

        avcodec_send_packet(audio->codec, avPacket);
        if (avcodec_receive_frame(audio->codec, audio->avFrame) != 0) {
            LOGE("解码失败");
            continue;
        }

        if (audio->isSilence) {
            set_silence_frame(audio->avFrame);
        }

        int nb = swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2,
                             (const uint8_t **) audio->avFrame->data, audio->avFrame->nb_samples);

        size = av_samples_get_buffer_size(NULL, audio->out_channer_nb, nb, AV_SAMPLE_FMT_S16, 1);
//            int len = nb * agrs->out_channer_nb * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        LOGE("getPcm nb=%d,size=%d,out_channer_nb=%d", nb, size, audio->out_channer_nb);

        if (audio->rate != 1) {
            nb = getSonicData(audio, nb);
            size = av_samples_get_buffer_size(NULL, audio->out_channer_nb, nb, AV_SAMPLE_FMT_S16, 1);
        }
        av_packet_unref(avPacket);
        av_packet_free(&avPacket);
        break;
    }


    av_frame_unref(audio->avFrame);

    return size;
}


//回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    //得到pcm数据
    LOGE("回调pcm数据")
    Audio *audio = (Audio *) context;
    int datasize = get_pcm(audio);
    if (datasize > 0) {
        //第一针所需要时间采样字节/采样率
        double time = datasize / (44100 * 2 * 2);

        audio->clock = time + audio->clock;
        LOGE("%d 当前一帧声音时间%f   播放时间%f", datasize, time, audio->clock);
        set_current_time(audio, audio->clock);

        if (audio->rate == 1) {
            (*bq)->Enqueue(bq, audio->out_buffer, datasize);
        } else {
            (*bq)->Enqueue(bq, audio->out_rate_buffer, datasize);
        }
    }

    if (!audio->status->isPlay) {
        LOGE("退出线程 audio")
        (*audio->androidJNI->pJavaVM)->DetachCurrentThread(audio->androidJNI->pJavaVM);
    }
}


void *ffp_start_audio_play(void *args) {
    Player *player = (Player *) args;
    Audio *audio = player->audio;
    bqPlayerCallback(audio->bqPlayerBufferQueue, audio);

    return 0;
}


bool create_openelse(Audio *audio) {
    LOGE("创建opnsl es播放器");
    //创建播放器
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&audio->engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现引擎engineObject
    result = (*audio->engineObject)->Realize(audio->engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 获取引擎接口engineEngine
    result = (*audio->engineObject)->GetInterface(audio->engineObject, SL_IID_ENGINE,
                                                  &audio->engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 创建混音器outputMixObject
    result = (*audio->engineEngine)->CreateOutputMix(audio->engineEngine, &audio->outputMixObject, 0,
                                                     0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现混音器outputMixObject
    result = (*audio->outputMixObject)->Realize(audio->outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    result = (*audio->outputMixObject)->GetInterface(audio->outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                     &audio->outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*audio->outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                audio->outputMixEnvironmentalReverb, &settings);
    }

    //======================
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
//   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
//    设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, audio->outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    //先讲这个
    (*audio->engineEngine)->CreateAudioPlayer(audio->engineEngine, &audio->bqPlayerObject, &slDataSource,
                                              &audioSnk, 2,
                                              ids, req);
    //初始化播放器
    (*audio->bqPlayerObject)->Realize(audio->bqPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*audio->bqPlayerObject)->GetInterface(audio->bqPlayerObject, SL_IID_PLAY, &audio->bqPlayerPlay);

//    注册回调缓冲区 //获取缓冲队列接口
    (*audio->bqPlayerObject)->GetInterface(audio->bqPlayerObject, SL_IID_BUFFERQUEUE,
                                           &audio->bqPlayerBufferQueue);
    //缓冲接口回调
    (*audio->bqPlayerBufferQueue)->RegisterCallback(audio->bqPlayerBufferQueue, bqPlayerCallback, audio);
//    获取音量接口
    (*audio->bqPlayerObject)->GetInterface(audio->bqPlayerObject, SL_IID_VOLUME, &audio->bqPlayerVolume);

//    获取播放状态接口
    (*audio->bqPlayerPlay)->SetPlayState(audio->bqPlayerPlay, SL_PLAYSTATE_PLAYING);

//    bqPlayerCallback(audio->bqPlayerBufferQueue, audio);

    return 1;
}
