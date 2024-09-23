
#include<google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <algorithm>  // 包含 std::generate_n() 和 std::generate() 函数的头文件
#include <functional>
#include <iostream>
#include <map>
#include <random>  // 包含 std::uniform_int_distribution 类型的头文件
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Channel : public google::protobuf::RpcChannel{
public:
    void CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request, google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;
    //构造函数
    Channel(string ip,short port,bool connectNow,int type);   //根据是注册服务还是调用服务来判断填1还是填0
private:
    //表示发送方的socket套接字
    int m_clientFd;
    int type=-1;
    int m_port;
    string m_ip;

    bool newConnect(const char * ip,int port,std::string * errMsg);
};
