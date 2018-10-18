//
// Created by Administrator on 2017/11/20.
//



#include "FFmpegMusic.h"

static void (*music_call)(double clock);

// 设置流的速率
int getSonicData(FFmpegMusic *music, int size, int nb) {
    //参数为采样率和声道数
    sonicSetSpeed(music->sonic, 1);
    sonicSetRate(music->sonic, music->rate);
    LOGE("music->rate=%f", music->rate)

    // 向流中写入pcm_buffer
    int ret = sonicWriteShortToStream(music->sonic, (short *) (music->out_buffer), nb);
    // 计算处理后的点数
    int new_buffer_size = 0;
    if (ret) {
        // 从流中读取处理好的数据
        new_buffer_size = sonicReadShortFromStream(music->sonic, music->out_rate_buffer, 44100 * 2);
        return new_buffer_size;
    }

    return 0;
}

//播放线程
void *MusicPlay(void *args) {
    FFmpegMusic *musicplay = (FFmpegMusic *) args;
    musicplay->avPacket = av_packet_alloc();
    musicplay->avFrame = av_frame_alloc();
    musicplay->CreatePlayer();

    pthread_exit(0);//退出线程
}

void set_silence_frame(AVFrame *frame) {
//    int32_t ret;

//    frame->sample_rate = samplerate;
//    frame->format = format; /*默认的format:AV_SAMPLE_FMT_FLTP*/
//    frame->channel_layout = av_get_default_channel_layout(channels);
//    frame->channels = channels;
//    frame->nb_samples = nb_samples; /*默认的sample大小:1024*/
//    ret = av_frame_get_buffer(frame, 0);
//    if (ret < 0) {
//        return;
//    }

    av_samples_set_silence(frame->data, 0, frame->nb_samples, frame->channels, (AVSampleFormat) frame->format);
}

//得到pcm数据
int getPcm(FFmpegMusic *agrs) {
    AVPacket *avPacket = agrs->avPacket;
    AVFrame *avFrame = agrs->avFrame;
    int size;
    LOGE("准备解码");
    while (agrs->isPlay) {
        size = 0;
        agrs->get(avPacket);
        //时间矫正
        if (avPacket->pts != AV_NOPTS_VALUE) {
            agrs->clock = av_q2d(agrs->time_base) * avPacket->pts;
        }
        LOGE("解码");
        avcodec_send_packet(agrs->codec, avPacket);
        if (avcodec_receive_frame(agrs->codec, avFrame) != 0) {
            continue;
        }
//        avcodec_decode_audio4(agrs->codec, avFrame, &gotframe, avPacket);

        if (agrs->isSilence) {
            set_silence_frame(avFrame);
        }

        int nb = swr_convert(agrs->swrContext, &agrs->out_buffer, 44100 * 2,
                             (const uint8_t **) avFrame->data, avFrame->nb_samples);

        size = av_samples_get_buffer_size(NULL, agrs->out_channer_nb, nb, AV_SAMPLE_FMT_S16, 1);
//            int len = nb * agrs->out_channer_nb * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        LOGE("getPcm nb=%d,size=%d,out_channer_nb=%d,len=%d", nb, size, agrs->out_channer_nb);

        if (agrs->rate != 1) {
            nb = getSonicData(agrs, size, nb);
            size = av_samples_get_buffer_size(NULL, agrs->out_channer_nb, nb, AV_SAMPLE_FMT_S16, 1);
        }
        break;
    }

    av_packet_unref(avPacket);
//    av_packet_free(&avPacket);
    av_frame_unref(avFrame);
//    av_frame_free(&avFrame);

    return size;
}

//回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    //得到pcm数据
    LOGE("回调pcm数据")
    FFmpegMusic *music = (FFmpegMusic *) context;
    int datasize = getPcm(music);
    if (datasize > 0) {
        //第一针所需要时间采样字节/采样率
        double time = datasize / (44100 * 2 * 2);

        music->clock = time + music->clock;
        LOGE("%d 当前一帧声音时间%f   播放时间%f", datasize, time, music->clock);
        if (music_call) {
            music_call(music->clock);
        }
        if (music->rate == 1) {
            (*bq)->Enqueue(bq, music->out_buffer, datasize);
        } else {
            (*bq)->Enqueue(bq, music->out_rate_buffer, datasize);
        }
//        LOGE("播放 %d ", music->queue.size());
    }
}

//初始化ffmpeg
int createFFmpeg(FFmpegMusic *music) {
    LOGE("初始化ffmpeg");
    music->swrContext = swr_alloc();

    int length = 0;
    int got_frame;
    //    44100*2
    music->out_buffer = (uint8_t *) av_mallocz(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //    输出采样位数  16位
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;
    //输出的采样率必须与输入相同
    int out_sample_rate = music->codec->sample_rate;
    swr_alloc_set_opts(music->swrContext, out_ch_layout, out_formart, out_sample_rate,
                       music->codec->channel_layout, music->codec->sample_fmt, music->codec->sample_rate, 0,
                       NULL);

    swr_init(music->swrContext);
    //    获取通道数  2
    music->out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("------>通道数%d  ", music->out_channer_nb);

    //倍速初始化
    music->out_rate_buffer = (short *) av_mallocz(44100 * 2);
    music->sonic = sonicCreateStream(music->codec->sample_rate, music->out_channer_nb);
    return 0;
}

FFmpegMusic::FFmpegMusic() {
    clock = 0;
    //初始化锁
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    queue = createQueue();
}

void FFmpegMusic::setAvCodecContext(AVCodecContext *avCodecContext) {
    codec = avCodecContext;
    createFFmpeg(this);
}

//将packet压入队列,生产者
int FFmpegMusic::put(AVPacket *avPacket) {
    LOGE("插入队列")
    AVPacket *avPacket1 = av_packet_alloc();
    //克隆
    if (av_packet_ref(avPacket1, avPacket)) {
        //克隆失败
        return 0;
    }
    //push的时候需要锁住，有数据的时候再解锁
    pthread_mutex_lock(&mutex);
    enQueue(queue,avPacket1);//将packet压入队列
    //压入过后发出消息并且解锁
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

//将packet弹出队列
int FFmpegMusic::get(AVPacket *avPacket) {
    LOGE("取出队列")
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (queue->size>0 && isPause) {
            //如果队列中有数据可以拿出来
            AVPacket *ptk = deQueue(queue);
            if (av_packet_ref(avPacket, ptk)) {
                break;
            }
            //取成功了，弹出队列，销毁packet
            av_packet_unref(ptk);
            av_packet_free(&ptk);
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

void FFmpegMusic::play() {
    isPause = true;
    isPlay = true;
    pthread_create(&playId, NULL, MusicPlay, this);//开启begin线程
}

FFmpegMusic::~FFmpegMusic() {
    if (out_buffer) {
        av_free(out_buffer);
    }
    if (out_rate_buffer) {
        av_free(out_rate_buffer);
    }
    AVPacket *pkt = deQueue(queue);
    while (pkt){
        LOGE("销毁帧%d",1);
        av_packet_unref(pkt);
        av_packet_free(&pkt);
        pkt = deQueue(queue);
    }
    freeQueue(queue);
//    (std::vector<AVPacket *>).swap(queue);
//    LOGE("帧空间=%d", queue.capacity());

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void FFmpegMusic::stop() {
    LOGE("声音暂停");
    //因为可能卡在 deQueue
    pthread_mutex_lock(&mutex);
    isPlay = false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(playId, 0);
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;

        bqPlayerBufferQueue = 0;
        bqPlayerVolume = 0;
    }

    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineEngine = 0;
    }
    if (swrContext) {
        swr_free(&swrContext);
    }
    if (this->codec) {
//        if (avcodec_is_open(this->codec)) {
//            avcodec_close(this->codec);
//        }
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGE("AUDIO clear");

    if (sonic) {
        sonicDestroyStream(sonic);
    }

    av_packet_unref(avPacket);
    av_packet_free(&avPacket);
    av_frame_unref(avFrame);
    av_frame_free(&avFrame);
}

int FFmpegMusic::CreatePlayer() {
    LOGE("创建opnsl es播放器")
    //创建播放器
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现引擎engineObject
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 获取引擎接口engineEngine
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 创建混音器outputMixObject
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0,
                                              0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &settings);
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
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    //先讲这个
    (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource,
                                       &audioSnk, 2,
                                       ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

//    注册回调缓冲区 //获取缓冲队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //缓冲接口回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
//    获取音量接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);

//    获取播放状态接口
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bqPlayerBufferQueue, this);

    return 1;
}

void FFmpegMusic::pause() {
    if (isPause) {
        isPause = false;
    } else {
        isPause = true;
        pthread_cond_signal(&cond);
    }
}

void FFmpegMusic::setPlayCall(void (*call)(double)) {
    music_call = call;
}


