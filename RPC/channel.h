
#include<google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <algorithm>  // ���� std::generate_n() �� std::generate() ������ͷ�ļ�
#include <functional>
#include <iostream>
#include <map>
#include <random>  // ���� std::uniform_int_distribution ���͵�ͷ�ļ�
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Channel : public google::protobuf::RpcChannel{
public:
    void CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request, google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;
    //���캯��
    Channel(string ip,short port,bool connectNow,int type);   //������ע������ǵ��÷������ж���1������0
private:
    //��ʾ���ͷ���socket�׽���
    int m_clientFd;
    int type=-1;
    int m_port;
    string m_ip;

    bool newConnect(const char * ip,int port,std::string * errMsg);
};
