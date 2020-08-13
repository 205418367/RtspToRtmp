#ifndef TRANS_STREAM_H
#define TRANS_STREAM_H
#include <memory>
#include <list>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "../Rtsp2Rtmp.h"
#include "PacketQueue.h"

using namespace std;

extern "C"{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
}

class TransStream
{
public:
    TransStream(ADDR_PARAM param);
    ~TransStream();
    void Init();
    int OpenInput();
    int OpenOutput();
    AVPacket* ReadPacket();
    void RescalePacketTs(AVPacket* packet,AVRational src_ts,AVRational dst_ts);
    int WritePacket(AVPacket* packet);
    int Start(const char* output_url = nullptr);
    int Stop();
    int OpenDecoder();
    int CapturePicture(const char* pic_path);
    int WriteJPG(AVFrame *frame,int width,int height);
    void RegisterCallBack(CallBackFunction callback_fun);
    int StartRecord(const char* file_path);
    int StopRecord();

private:
    void ThreadTransStream();
    void ThreadRelyCapurePic();//实时流抓图
    void ThreadAloneCapurePic();//无视频时，单独抓图
    void ThreadRecordFile();//本地录像
    void StopPacketQueue();
    void StopRecordQueue();
    bool IsKeyFrame(AVPacket* packet);

private:
    ADDR_PARAM addr_param_;
    AVFormatContext* input_context_;
    AVFormatContext* output_context_;
    AVCodecContext* codec_context_;
    bool ffmpeg_inited_;//是否已经初始化了
    bool input_inited_;//输入是否初始化了
    bool stop_;//是否停止视频实时浏览
    bool capture_;//是否停止抓图
    bool record_;//是否需要录像
    int video_index_;//只处理视频，忽略音频
    char* capture_path_;
    std::string record_file_;//录像文件
	std::mutex mutex_stop_;
    std::mutex mutex_capture_;
    std::mutex mutex_record_;
    std::thread* thread_capture_;
    std::thread* thread_trans_stream_;
    PacketQueue<AVPacket*> packet_queue_;
    PacketQueue<AVPacket*> record_queue_;//录像文件
    CallBackFunction callback_fun_;

    std::mutex mutex_synchro_capture_;
    std::condition_variable cond_synchro_capture_;
    bool cond_param_capture_;

    std::mutex mutex_synchro_stop_;
    std::condition_variable cond_synchro_stop_;
    bool cond_param_stop_;
};

#endif
