#include "TransStream.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern const int capture_timeout;

#define FILE_PATH_LENGTH 1024

 TransStream::TransStream(ADDR_PARAM param)
 {
     input_context_ = nullptr;
     output_context_ = nullptr;
	 codec_context_ = nullptr;
     addr_param_ = param;
	 stop_ = true;
     capture_ = false;
     video_index_ = -1;
	 capture_path_ = new char[FILE_PATH_LENGTH];
     thread_capture_ = nullptr;
     thread_trans_stream_ = nullptr;
     ffmpeg_inited_ = false;
     input_inited_ = false;
     callback_fun_ = nullptr;
     cond_param_capture_ = false;
     cond_param_stop_ = false;
     record_ = false;
 }

 TransStream::~TransStream()
 {
   if(capture_path_){
	   delete[] capture_path_;
   }
   capture_path_ = nullptr;

   if(thread_capture_&&thread_capture_->joinable()){
       thread_capture_->detach();
   }
   thread_capture_ = nullptr;

   if (thread_trans_stream_&&thread_trans_stream_->joinable()){
       thread_trans_stream_->detach();
   }
   thread_trans_stream_ = nullptr;
 }

void TransStream::Init()
{
    if(!ffmpeg_inited_){
       av_register_all();
       avformat_network_init();
       ffmpeg_inited_ = true;
    }
}

int TransStream::OpenInput()
{
    input_context_ = avformat_alloc_context();
    AVDictionary* options = nullptr; 
//	av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "rtsp_transport", "udp", 0);//传输协议
    av_dict_set(&options, "stimeout", "3000000", 0);//超时时间，单位：us
	int ret = avformat_open_input(&input_context_, addr_param_.input_rtsp_addr, nullptr,&options);
	if(ret < 0)
	{
#ifdef PRINT_LOG
    LOG_ERROR("open input error,error code %d",ret);
#endif
        avformat_close_input(&input_context_);
		return ERROR_OPEN_INPUT_RTSP;
	}
    ret = avformat_find_stream_info(input_context_,nullptr);
    if (ret < 0){
#ifdef PRINT_LOG
    LOG_ERROR("find stream info error,error code %d",ret);
#endif
        avformat_close_input(&input_context_);
        return ret;
    }

    for(int i = 0 ; i < input_context_->nb_streams; i++){
        if(input_context_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index_ = i;
            codec_context_ = input_context_->streams[i]->codec;
            break;
        }
    }
    return ret >= 0? 0:ret;
}

int TransStream::OpenOutput()
{
    int ret = avformat_alloc_output_context2(&output_context_,nullptr,"flv",addr_param_.output_rtmp_addr);
    if(ret < 0){
#ifdef PRINT_LOG
    LOG_ERROR("alloc output context error,error code %d",ret);
#endif
        avformat_close_input(&output_context_);
        return ret;
    }
    AVDictionary* options = nullptr; 
    av_dict_set(&options, "stimeout", "3000000", 0);//超时时间，单位：us
    ret = avio_open2(&output_context_->pb, addr_param_.output_rtmp_addr, AVIO_FLAG_READ_WRITE,nullptr, &options/*nullptr*/);	
	if(ret < 0){
#ifdef PRINT_LOG
    LOG_ERROR("avio open error,error code %d",ret);
#endif
        avformat_close_input(&output_context_);
		return ERROR_OPEN_RTMP_SERVER;
	}
    
    for(int i = 0; i < input_context_->nb_streams; i++){
		AVStream * stream = avformat_new_stream(output_context_, input_context_->streams[i]->codec->codec);		
        ret = avcodec_parameters_from_context(stream->codecpar, input_context_->streams[i]->codec);
        av_stream_set_r_frame_rate(stream, { 1, 25 });		
//		ret = avcodec_copy_context(stream->codec, input_context_->streams[i]->codec);	
		if(ret < 0){
#ifdef PRINT_LOG
    LOG_ERROR("copy codec context error,error code %d",ret);
#endif
			goto ERROR;
		}
	}

	ret = avformat_write_header(output_context_, nullptr);
	if(ret < 0){
#ifdef PRINT_LOG
    LOG_ERROR("write stream header error,error code %d",ret);
#endif
		goto ERROR;
	}
    return ret >= RET_OK? 0:ret;
ERROR:
    if(output_context_){
		for(int i = 0; i < output_context_->nb_streams; i++)
		{
			avcodec_close(output_context_->streams[i]->codec);
		}
		avformat_close_input(&output_context_);
	}
	return ret ;
}

AVPacket* TransStream::ReadPacket()
{
    AVPacket* packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))));
	av_init_packet(packet);
	int ret = av_read_frame(input_context_, packet);
	if(ret >= 0)
	{
		return packet;
	}
	else
	{
		return nullptr;
	}
}

void TransStream::RescalePacketTs(AVPacket* packet,AVRational src_ts,AVRational dst_ts)
{
    if(packet->pts != AV_NOPTS_VALUE){
        packet->pts = av_rescale_q(packet->pts,src_ts,dst_ts);
    }
    if(packet->dts != AV_NOPTS_VALUE){
        packet->dts = av_rescale_q(packet->dts,src_ts,dst_ts);
    }
    if(packet->duration > 0){
        packet->duration = av_rescale_q(packet->duration,src_ts,dst_ts);
    }
}

int TransStream::WritePacket(AVPacket* packet)
{
    do{
        std::unique_lock<std::mutex> lock(mutex_stop_);
        if(true == stop_){
            return RET_OK;
        }
    }while(false);
    auto inputStream = input_context_->streams[packet->stream_index];
	auto outputStream = output_context_->streams[packet->stream_index];				
    RescalePacketTs(packet,inputStream->time_base,outputStream->time_base);
	return av_interleaved_write_frame(output_context_, packet);
}

//用一个线程去执行TransStream类的ThreadTransStream成员函数
int TransStream::Start(const char* output_url)
{
    bool stop = true;
    do{ //unique_lock的有效范围在do{}while()循环体内
        std::unique_lock<std::mutex> lock(mutex_stop_);
        stop = stop_;
    }while(false);
    
    if(false == stop){
        return ERROR_REALVIDEO_ALREADY_STARTED;
    }
    if(nullptr != output_url){
        memcpy(addr_param_.output_rtmp_addr,output_url,strlen(output_url) + 1);
    }
    if(nullptr == addr_param_.input_rtsp_addr||nullptr == addr_param_.output_rtmp_addr){
        return ERROR_PARAM;
    }

    Init();//ffmpeg初始化
    int ret = OpenInput();
    if(ret != RET_OK){
        return ret;
    }
    ret = OpenOutput();
    if(ret != RET_OK){
        return ret;
    }
    //unique_lock析构时解锁
    std::unique_lock<std::mutex> lock(mutex_stop_);
    stop_ = false;
    //lock.unlock(); //临时解锁
    //lock.lock();   //临时上锁
    std::thread thr_trans_stream(&TransStream::ThreadTransStream,this);
    thr_trans_stream.detach();
    return RET_OK;
}

int TransStream::Stop()
{
    do{
        std::unique_lock<std::mutex> lock(mutex_record_);
        if(true == record_){
            record_ = false;
        }
    }while(false);
	
    do{
        std::unique_lock<std::mutex> lock(mutex_stop_);
        if(true == stop_){
            return ERROR_REALVIDEO_ALREADY_STOPED;
        }
        stop_ = true;
    }while(false);
    
    std::unique_lock<std::mutex> lock(mutex_synchro_stop_);
    cond_param_stop_ = false;

    auto fun = [&]()->bool{return cond_param_stop_ == true;};
    cond_synchro_stop_.wait_for(lock, std::chrono::milliseconds(stop_timeout*1000), fun);
    return RET_OK;
}

int TransStream::OpenDecoder()
{
    AVCodec* codec=avcodec_find_decoder(codec_context_->codec_id);
    if(codec == nullptr)
    {
        return ERROR_FIND_DECODER;
    }
    if(avcodec_open2(codec_context_, codec,nullptr)<0)
    {
        return ERROR_OPEN_DECODER;
    }
    return RET_OK;
}

int TransStream::CapturePicture(const char* pic_path)
{
    bool stop = false;
    do
    {
        std::unique_lock<std::mutex> locker(mutex_stop_);
        stop = stop_;
    } while (false);
    
    memset(capture_path_,0,FILE_PATH_LENGTH);
    memcpy(capture_path_ ,pic_path,strlen(pic_path) + 1);

    //1、如果没有实时浏览视频
    if(stop){ 
        Init();//ffmpeg初始化
        int ret = OpenInput();
        if(ret != RET_OK){
            return ret;
        }
        if(nullptr == codec_context_){
            return ERROR_INPUT_SOURCE;
        }
        ret = OpenDecoder();
        if(ret != RET_OK){
            return ret;
        }
        std::thread thr_relay_capture(&TransStream::ThreadRelyCapurePic,this);
        thr_relay_capture.detach();
    }
    //2、如果正在实时浏览
    else{
        int ret = OpenDecoder();
        if(ret != RET_OK){
            return ret;
        }
        if(nullptr == codec_context_){
            return ERROR_INPUT_SOURCE;
        }
        std::unique_lock<std::mutex> lock(mutex_capture_);
        capture_ = true;
        std::thread thr_alone_capture(&TransStream::ThreadAloneCapurePic,this);
        thr_alone_capture.detach();
    }
    std::unique_lock<std::mutex> lock(mutex_synchro_capture_);
    cond_param_capture_ = false;
    auto fun = [&]()->bool{return cond_param_capture_ == true;};
    //wait_for 可以指定一个时间段，在当前线程收到通知或者指定的时间 rel_time 超时之前，该线程都会处于阻塞状态。
    //而一旦超时或者收到了其他线程的通知，wait_for 返回true。fun为预测条件。
    if(cond_synchro_capture_.wait_for(lock, std::chrono::milliseconds(capture_timeout*1000), fun)){    
        std::unique_lock<std::mutex> lock_capture(mutex_capture_);
        capture_  = false;
        return RET_OK;
    }
    std::unique_lock<std::mutex> lock_capture(mutex_capture_);
    capture_  = false;
    return ERROR_CAPTURE_TIMEOUT;
}

void TransStream::ThreadTransStream()
{
    while(!stop_){
		AVPacket* packet = ReadPacket();
        if(packet && packet->stream_index == video_index_){ //这里是视频
            bool capture = false;
            do{
                std::unique_lock<std::mutex> lock(mutex_capture_); 
                capture = capture_; 
            }while(false);

            bool record = false;
            do{
                std::unique_lock<std::mutex> lock(mutex_record_); 
                record = record_; 
            }while(false);

            if(record){//录像
                AVPacket* tmp_packet = av_packet_alloc();
                av_init_packet(tmp_packet);
				av_packet_ref(tmp_packet, packet);
                record_queue_.AddData(tmp_packet);
            }

            if(capture&&IsKeyFrame(packet)){
                //为啥要加这个呢？因为：shared_ptr<AVPacket> packet和实际的内存数据是独立的，如果不将内存数据av_packet_ref加1，则其在packet作用局结束后，自动销毁
                AVPacket* tmp_packet = av_packet_alloc();
                av_init_packet(tmp_packet);
				av_packet_ref(tmp_packet, packet);
               
                WritePacket(packet);
                av_packet_unref(packet);
                av_freep(&packet);

                packet_queue_.AddData(tmp_packet);
            }
            else{
                WritePacket(packet);
               
                av_packet_unref(packet);
                av_freep(&packet);
            }
        }
        else if(packet){ //这里是音频
            WritePacket(packet);
            av_packet_unref(packet);
            av_freep(&packet);
        }
    }

	for(int i = 0; i < output_context_->nb_streams; i++){
        avcodec_close(output_context_->streams[i]->codec);
    }
    if(codec_context_){
        avcodec_close(codec_context_);
    }
	avformat_close_input(&input_context_);
	avformat_close_input(&output_context_);

    cond_param_stop_ = true;
    cond_synchro_stop_.notify_one();
}

void TransStream::ThreadRelyCapurePic()
{
    AVFrame* frame = av_frame_alloc();
    while(true){
        AVPacket* packet = ReadPacket();
        if(packet&& packet->stream_index == video_index_){
            int gotFrame = 0;
            int ret = avcodec_send_packet(codec_context_,packet);
            ret = avcodec_receive_frame(codec_context_,frame);
            if(0 == ret){
                ret = WriteJPG(frame,codec_context_->width,codec_context_->height);
                if(0 == ret){
                    av_packet_unref(packet);
                    av_freep(&packet);
                    break;
                }
            }
            av_frame_unref(frame);
            av_packet_unref(packet);
            av_freep(&packet);
        }
        else if(packet){
            av_packet_unref(packet);
            av_freep(&packet);
        }
        else{
            break;
        }
    }
    if(frame){ //如果解码失败，则frame没有释放
        av_frame_unref(frame);
        av_freep(&frame);
    }
   
    if(codec_context_){
        avcodec_close(codec_context_);
    }
    if(input_context_){
        avformat_close_input(&input_context_);
    }
}

void TransStream::ThreadAloneCapurePic()
{
    while(true){
        AVPacket* packet = nullptr;
        bool ret = packet_queue_.GetData(packet);
        if (ret && packet){ //获取到数据了
            int gotFrame = 0;
            AVFrame* frame = av_frame_alloc();
            int ret = avcodec_send_packet(codec_context_,packet);
            ret = avcodec_receive_frame(codec_context_,frame);
//          ret = avcodec_decode_video2(codec_context_,frame,&gotFrame,packet_tmp);
            if(0 == ret){
                ret = WriteJPG(frame,codec_context_->width,codec_context_->height);
                if(0 == ret){ //写图片成功
                    av_frame_unref(frame);
                    av_freep(&frame);
                    av_packet_unref(packet);
                    av_freep(&packet);
                    break;
                }
            }
            if(frame){ //如果解码失败，则frame没有释放
                av_frame_unref(frame);
                av_freep(&frame);
            }
            av_packet_unref(packet);
            av_freep(&packet);
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if(codec_context_){
        avcodec_close(codec_context_);
    }
    StopPacketQueue();
}

void TransStream::StopPacketQueue()
{
    int size = packet_queue_.Size();
    if(size >= 1){
        std::list<AVPacket*> list;
        packet_queue_.GetDataList(list);
        auto it_list = list.begin();
        while(it_list != list.end())
        {
            AVPacket *packet = *it_list;
            if(*it_list != nullptr){
                it_list = list.erase(it_list);
            }
            else{
                it_list++;
            }
            if(packet){
                av_packet_unref(packet);
                av_freep(&packet);
            }
        }
    }
    packet_queue_.Clear();
}

void TransStream::StopRecordQueue()
{
    int size = record_queue_.Size();
    if(size >= 1){
        std::list<AVPacket*> list;
        record_queue_.GetDataList(list);
        auto it_list = list.begin();
        while(it_list != list.end())
        {
            AVPacket *packet = *it_list;
            if(*it_list != nullptr){
                it_list = list.erase(it_list);
            }
            else{
                it_list++;
            }
            if(packet){
                av_packet_unref(packet);
                av_freep(&packet);
            }
        }
    }
    record_queue_.Clear();
}

bool TransStream::IsKeyFrame(AVPacket* packet)
{
    if (packet){
		if (packet->flags & AV_PKT_FLAG_KEY){
			return true;
		}
	}
	return false;
}

int TransStream::WriteJPG(AVFrame *frame,int width,int height)
{
    const char* out_file = capture_path_;
   //新建一个输出的AVFormatContext 并分配内存
    AVFormatContext* output_cxt = nullptr;
    avformat_alloc_output_context2(&output_cxt,NULL,"singlejpeg",out_file);

    //设置输出文件的格式
    // output_cxt->oformat = av_guess_format("mjpeg",NULL,NULL);

    //创建和初始化一个和该URL相关的AVIOContext
    if(avio_open(&output_cxt->pb,out_file,AVIO_FLAG_READ_WRITE) < 0){
        av_log(NULL,AV_LOG_ERROR,"不能打开文件  \n");
        return -1;
    }

    //构建新的Stream
    AVStream* stream = avformat_new_stream(output_cxt,NULL);
    if(stream == NULL){
        av_log(NULL,AV_LOG_ERROR,"创建AVStream失败  \n");
        return -1;
    }
    //初始化AVStream信息
    AVCodecContext* codec_cxt = stream->codec;

    codec_cxt->codec_id = output_cxt->oformat->video_codec;
    codec_cxt->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_cxt->pix_fmt = AV_PIX_FMT_YUVJ420P;
    codec_cxt->height = height;
    codec_cxt->width = width;
    codec_cxt->time_base.num = 1;
    codec_cxt->time_base.den = 25;

    //打印输出文件信息
    av_dump_format(output_cxt,0,out_file,1);

    AVCodec* codec = avcodec_find_encoder(codec_cxt->codec_id);
    if(!codec){
        av_log(NULL,AV_LOG_ERROR,"找不到编码器  \n");
        return -1;
    }

    if(avcodec_open2(codec_cxt,codec,NULL) < 0){
        av_log(NULL,AV_LOG_ERROR,"不能打开编码器  \n");
        return -1;
    }
    avcodec_parameters_from_context(stream->codecpar,codec_cxt);

    //写入文件头
    avformat_write_header(output_cxt,NULL);
    int size = codec_cxt->width * codec_cxt->height;

    std::shared_ptr<AVPacket>pkt(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))),[&](AVPacket* p){av_packet_free(&p);av_freep(&p);});
    av_init_packet(pkt.get());
    pkt->data = nullptr;
    pkt->size = 0;

    int got_picture = 0;
    int result = avcodec_encode_video2(codec_cxt,pkt.get(),frame,&got_picture);
    if(result < 0){
        av_log(NULL,AV_LOG_ERROR,"编码失败  \n");
        return -1;
    }
    printf("got_picture %d \n",got_picture);
    if(got_picture == 1){
        //将packet中的数据写入本地文件
        result = av_write_frame(output_cxt,pkt.get());
    }
 
    //将流尾写入输出媒体文件并释放文件数据
    av_write_trailer(output_cxt);

    avcodec_close(codec_cxt);
    avformat_close_input(&output_cxt);

    cond_param_capture_ = true;
    cond_synchro_capture_.notify_one();

    return RET_OK;
}

void TransStream::RegisterCallBack(CallBackFunction callback_fun)
{
    callback_fun_ = callback_fun;
}

int TransStream::StartRecord(const char* file_path)
{
    record_file_ = file_path;
    if(stop_){
        return ERROR_NO_REALVIDEO;
    }
    std::unique_lock<std::mutex> lock(mutex_record_);
    if(true == record_){
        return ERROR_RECORD_ALREADY_STARTED;
    }
    record_ = true;
    std::thread thr_record(&TransStream::ThreadRecordFile,this);
    thr_record.detach();
    return RET_OK;
}

int TransStream::StopRecord()
{
    std::unique_lock<std::mutex> lock(mutex_record_);
    if(false == record_){
        return ERROR_RECORD_ALREADY_STOPED;
    }
    record_ = false;
    return RET_OK;
}

void TransStream::ThreadRecordFile()
{
    AVFormatContext * file_context = nullptr;
    AVBitStreamFilterContext* bsfc  = nullptr;
    do{   
        int ret = avformat_alloc_output_context2(&file_context,nullptr,nullptr,record_file_.c_str());
        if(ret < 0){
            break;
        }
        
        bool record = true;
        do{
            std::unique_lock<std::mutex> lock(mutex_record_);
            record = record_;
        }while(false);
        if(false == record){
            avformat_close_input(&file_context);
            StopRecordQueue();
            return;
        }

        //输出依赖于输入
        AVStream * stream = avformat_new_stream(file_context, input_context_->streams[video_index_]->codec->codec);   
        ret = avcodec_parameters_from_context(stream->codecpar,input_context_->streams[video_index_]->codec);          
//      ret = avcodec_copy_context(stream->codec, input_context_->streams[i]->codec); 
        if(ret < 0)
        {
            break;
        }
        stream->codec->codec_tag = 0;
        if(file_context->oformat->flags&AVFMT_GLOBALHEADER){
            stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        int file_video_index = stream->index;
        
        ret = avio_open(&file_context->pb,record_file_.c_str(),AVIO_FLAG_READ_WRITE);
        if(ret < 0){
            break;
        }

        ret = avformat_write_header(file_context, nullptr);
        if(ret < 0){
            break;
        }
        bsfc =  av_bitstream_filter_init("h264_mp4toannexb");

        record = true; 
        bool stop = false;
        while(record&&!stop){
            do{
                std::unique_lock<std::mutex> lock(mutex_record_);
                record = record_;
            }while(false);

            do{
                std::unique_lock<std::mutex> lock(mutex_stop_);
                stop = stop_;
            }while(false);

            AVPacket* packet = nullptr;
            bool ret = record_queue_.GetData(packet);
            if (ret && packet){ //获取到数据了
                auto inputStream = input_context_->streams[video_index_];
                auto outputStream = file_context->streams[file_video_index];
                av_packet_rescale_ts(packet,inputStream->time_base,outputStream->time_base);//时间戳转换，输入上下文与输出上下文时间基准不同
                av_bitstream_filter_filter(bsfc, inputStream->codec, NULL, &packet->data, &packet->size, packet->data, packet->size, packet->flags & AV_PKT_FLAG_KEY);
                av_write_frame(file_context,packet);

                av_packet_unref(packet);
                av_freep(&packet);
            }
            else{
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }while(false);

    av_write_trailer(file_context);
    av_bitstream_filter_close(bsfc);
    
    if(file_context){
        avformat_close_input(&file_context);
    }

    StopRecordQueue();

    std::unique_lock<std::mutex> lock(mutex_record_);
    record_ = false;
}
