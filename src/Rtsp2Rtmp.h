#ifndef RTSP_2_RTMP_H
#define RTSP_2_RTMP_H
#include <string.h>
#include "IRtsp2Rtmp.h"

#define INPUT_RTSP_LENGTH 1024
#define OUTPUT_RTMP_LENGTH 1024

typedef struct Addr_Param_{
	char input_rtsp_addr[INPUT_RTSP_LENGTH];
	char output_rtmp_addr[OUTPUT_RTMP_LENGTH];
        //初始化
	Addr_Param_(){
		memset(input_rtsp_addr, 0, INPUT_RTSP_LENGTH);
		memset(output_rtmp_addr, 0, OUTPUT_RTMP_LENGTH);
	}
}ADDR_PARAM;

//TransStream与Rtsp2Rtmp并非继承关系
class TransStream;
class Rtsp2Rtmp:public IRtsp2Rtmp
{
public:
	Rtsp2Rtmp();
	~Rtsp2Rtmp();
        //虚拟函数衍生下去仍然是虚拟函数，而且还可以省略掉关键字“virtual”
	virtual int Init(const char* input_url,const char* output_url = nullptr);
	virtual int Start(const char* output_url = nullptr);//start trans stream
	virtual int CapturePicture(const char* pic_path);
	virtual int Stop();
	virtual int StartRecord(const char* file_path);//start real video record
	virtual int StopRecord();//stop real video record
	virtual void RegisterCallBack(CallBackFunction callback_fun);//registe status call back
private:
        //主要功能实现在TransStream类
	TransStream *trans_stream_;
};

#endif
