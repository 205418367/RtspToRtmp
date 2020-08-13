#ifndef I_RTSP_2_RTMP_H
#define I_RTSP_2_RTMP_H
#include <stdio.h>

const int capture_timeout = 3;//抓拍超时时间，单位秒
const int stop_timeout = 2;//停止超时时间，单位秒

enum ERROR_CODE //接口返回值
{
	RET_OK = 0,//返回正确
	ERROR_PARAM = 1,//初始化参数错误(rtsp拉流地址为空，或者rtmp推流地址为空)
	ERROR_MEMORY = 2,//初始化时，内存不足
	ERROR_FIND_DECODER = 3,//查找解码器失败
	ERROR_OPEN_DECODER = 4,//打开解码器失败
	ERROR_INPUT_SOURCE = 5,//获取输入视频源失败
	ERROR_ALREADY_INITED = 6,//重复初始化了
	ERROR_CAPTURE_TIMEOUT = 7,//抓拍超时
	ERROR_NO_REALVIDEO = 8,//这里做了限制，没有实时浏览前，无法录像
	ERROR_RECORD_ALREADY_STARTED = 9,//录像已经开始
	ERROR_RECORD_ALREADY_STOPED = 10,//录像已经停止
	ERROR_REALVIDEO_ALREADY_STARTED = 11,//实时视频已经开启
	ERROR_REALVIDEO_ALREADY_STOPED = 12,//实时视频已经停止
	ERROR_OPEN_INPUT_RTSP = 13,//获取rtsp视频流失败
	ERROR_OPEN_RTMP_SERVER = 14,//连接rtmp服务器失败
};

enum CALLBACK_MSG_TYPE //回调函数类型
{
	MSG_CAPTURE_SUCCEED = 0,//抓拍成功
	MSG_CAPTURE_FAILED = 1,//抓拍失败
	MSG_RECORD_FAILED = 2,//录像失败
};

typedef void (*CallBackFunction)(CALLBACK_MSG_TYPE msg_type,const char* msg);

class IRtsp2Rtmp
{
public:
	virtual int Init(const char* input_url,const char* output_url = nullptr) = 0;//input_url：camera url output_url:rtmp server url
	virtual int Start(const char* output_url = nullptr) = 0;//start trans stream
	virtual int CapturePicture(const char* pic_path) = 0;//pic_path:the path to save captured picture
	virtual int Stop() =0;
	virtual int StartRecord(const char* file_path) = 0;//start real video record
	virtual int StopRecord() = 0;//stop real video record
};

IRtsp2Rtmp* CreateTransServer();
void DestroyTransServer(IRtsp2Rtmp* i_server);

#endif
