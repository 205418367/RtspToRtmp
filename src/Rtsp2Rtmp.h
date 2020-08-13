#ifndef RTSP_2_RTMP_H
#define RTSP_2_RTMP_H
#include <string.h>
#include "IRtsp2Rtmp.h"
//#include "TransStream/TransStream.h"

#define INPUT_RTSP_LENGTH 1024
#define OUTPUT_RTMP_LENGTH 1024

typedef struct Addr_Param_{
	char input_rtsp_addr[INPUT_RTSP_LENGTH];//ip
	char output_rtmp_addr[OUTPUT_RTMP_LENGTH];//rtsp
	Addr_Param_(){
		memset(input_rtsp_addr, 0, INPUT_RTSP_LENGTH);
		memset(output_rtmp_addr, 0, OUTPUT_RTMP_LENGTH);
	}
}ADDR_PARAM;

class TransStream;
class Rtsp2Rtmp:public IRtsp2Rtmp
{
public:
	Rtsp2Rtmp();
	~Rtsp2Rtmp();
        //虚拟函数衍生下去仍然是虚拟函数，而且还可以省略掉关键字“virtual”
	virtual int Init(const char* input_url,const char* output_url = nullptr);//input_url：camera url     output_url:rtmp server url
	virtual int Start(const char* output_url = nullptr);//start trans stream
	virtual int CapturePicture(const char* pic_path);//pic_path:the path to save captured picture
	virtual int Stop();
	virtual int StartRecord(const char* file_path);//start real video record
	virtual int StopRecord();//stop real video record
	virtual void RegisterCallBack(CallBackFunction callback_fun);//registe status call back

private:
	TransStream *trans_stream_;
};

#endif
