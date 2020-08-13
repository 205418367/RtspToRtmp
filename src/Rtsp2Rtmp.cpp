#include <stdio.h>
#include <unistd.h>
//#include "Logger.h"
#include <thread>
#include <string>
#include "TransStream/TransStream.h"

#define MAX_PATH_LENGTH 1024

//Rtsp2Rtmp子类 IRtsp2Rtmp父类(抽象类）只有函数定义（纯虚函数）没有函数实现
//IRtsp2Rtmp* 抽象类不能被实例化，不过可以拥有指向抽象类的指针，以便于操纵各个衍生类
IRtsp2Rtmp* CreateTransServer(){
    IRtsp2Rtmp* trans_server = new Rtsp2Rtmp();
    return trans_server;
}

void DestroyTransServer(IRtsp2Rtmp* i_server){
    if(i_server){
        Rtsp2Rtmp* trans_server = static_cast<Rtsp2Rtmp*>(i_server);
        if(trans_server){
            delete trans_server;
            trans_server = nullptr;
        }
    }
}

Rtsp2Rtmp::Rtsp2Rtmp(){
    trans_stream_ = nullptr;
}

Rtsp2Rtmp::~Rtsp2Rtmp()
{
    if(trans_stream_){
        delete trans_stream_;
        trans_stream_ = nullptr;
    }
}

int Rtsp2Rtmp::Init(const char* input_url,const char* output_url)
{ 
    if(trans_stream_){//如果已经创建了
        return ERROR_ALREADY_INITED;
    }

    if(nullptr == input_url){
#ifdef PRINT_LOG
    if(nullptr == input_url){
        LOG_WARN("param error,input url is null!");
    }
    else{
        LOG_WARN("param error,output url is null!");
    }
#endif
        return ERROR_PARAM;
    }

    ADDR_PARAM addr_param;
    memcpy(addr_param.input_rtsp_addr,input_url,INPUT_RTSP_LENGTH);
    if(nullptr != output_url){
        memcpy(addr_param.output_rtmp_addr,output_url,OUTPUT_RTMP_LENGTH);
    }

    trans_stream_ = new TransStream(addr_param);
    if(nullptr == trans_stream_){
#ifdef PRINT_LOG        
        LOG_WARN("new TransStream object error!");
#endif
        return ERROR_MEMORY;
    }
    return RET_OK;
}

int Rtsp2Rtmp::Start(const char* output_url)
{
    if(trans_stream_){
        return trans_stream_->Start(output_url);
    }
    return ERROR_MEMORY;
}

int Rtsp2Rtmp::CapturePicture(const char* pic_path)
{
    if(trans_stream_){
	    return trans_stream_->CapturePicture(pic_path);
    }
    return ERROR_MEMORY;  
}

int Rtsp2Rtmp::Stop()
{
    if(trans_stream_){
        return trans_stream_->Stop();
    }
    return ERROR_MEMORY;
}

int Rtsp2Rtmp::StartRecord(const char* file_path)
{
     if(trans_stream_){
        trans_stream_->StartRecord(file_path);
    }
    return ERROR_MEMORY;
}

int Rtsp2Rtmp::StopRecord()
{
     if(trans_stream_){
        trans_stream_->StopRecord();
    }
    return ERROR_MEMORY;
}

void Rtsp2Rtmp::RegisterCallBack(CallBackFunction callback_fun)
{
     if(trans_stream_){
        
    }
}

//获取当前目录绝对路径，即去掉程序名
std::string GetAppDirect(){
    char app_path[MAX_PATH_LENGTH];
    memset(app_path,0,MAX_PATH_LENGTH);
    int cnt = readlink("/proc/self/exe", app_path, MAX_PATH_LENGTH);
    if (cnt < 0 || cnt >= MAX_PATH_LENGTH){
       return "";
    }
    int i;
    for (i = cnt; i >=0; --i){
        if (app_path[i] == '/')
        {
            app_path[i+1] = '\0';
            break;
        }
    }
    return std::string(app_path);
}

int main(int argc, char* argv[]){
    //获取app路径
    std::string app_dir = GetAppDirect();
    app_dir += "pic/";

    IRtsp2Rtmp* rtsp_trans = CreateTransServer();
    if(nullptr == rtsp_trans){
        return 1;
    }
    rtsp_trans->Init("rtsp://admin:leinao123@192.168.8.220/live0","rtmp://192.168.8.101/live/stream");

    std::string pic_name1 = app_dir + "1.jpg";
    rtsp_trans->CapturePicture(pic_name1.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(10));

    rtsp_trans->Start();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::string record_file = app_dir + "test.mp4";
    rtsp_trans->StartRecord(record_file.c_str());

    rtsp_trans->Stop();

    rtsp_trans->Start();
    bool record = false;
    int pic_index = 1;
    char pic_name[10];
    char record_name[10];
    while(true){
        memset(pic_name,0,10);
        sprintf(pic_name,"%d.jpg",++pic_index);
        std::string pic_path = app_dir + pic_name;
        rtsp_trans->CapturePicture(pic_path.c_str());
        
 /*       if(!record){
            if(pic_index % 20 == 0){
                memset(record_name,0,10);
                sprintf(record_name,"%d.mp4",pic_index);
                std::string record_path = app_dir + record_name;
                rtsp_trans->StartRecord(record_path.c_str());
                record = !record;
            }
        }
        else{
            if(pic_index % 20 == 0){
                rtsp_trans->StopRecord();
                record = !record;
            }
        }*/
        
        std::this_thread::sleep_for(std::chrono::seconds(5));//5秒钟抓图一次
    }
//    rtsp_trans->StopRecord();
    printf("********************************test compelete*******************************\n");
    system("pause");
}

