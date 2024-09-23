#include"channel.h"
#include<arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include<muduo/base/Logging.h>

//�����ǰ���util��
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <condition_variable>  // pthread_condition_t
#include <functional>
#include <iostream>
#include <mutex>  // pthread_mutex_t
#include <queue>
#include <random>
#include <sstream>
#include <thread>
//�������

#include"rpcheader.pb.h"
#include"control.h"
#include"../util/config.h"

using namespace std;

//(�ص㣺����rpc�ķ���protobuf��������Ҫ�б߽磬��ֹtcp��ճ�������⣬��Ȼ��ʱǰ��Ĳ������Լ����õ�ͷ��������ʶ�߽磩������Ĳ������壬������������
void Channel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller,
                         const google::protobuf::Message *request, google::protobuf::Message *response,
                         google::protobuf::Closure *done) {
    if(m_clientFd==-1 ){
        std::string errMsg;
        bool rt= newConnect(m_ip.c_str(),m_port,&errMsg);
        if(!rt){
            controller->SetFailed(errMsg);
            return ;
        }
    }
    ////��ȡ��ǰ��������ƣ��Լ������Ӧ��ĳ��������
    ////�õ�һ������������֪��һЩ��Ϣ
    const google::protobuf::ServiceDescriptor * sd=method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();
    //��ȡ���������л��ַ�������
    uint32_t args_size{};
    std::string args_str;
    //��request�����л����ַ��������ת�����ַ�������˼��ת����protobuf���͵�string�������Ǵ�protobufת����string�������浽args_str�У������ȡ����
    if(request->SerializeToString(&args_str) ){
        args_size=args_str.size();
    }else {
        controller->SetFailed("serialize request error!");
        return;
    }

    RPC::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);
    rpcHeader.set_message_type(this->type);
    rpcHeader.set_port(m_port);
    rpcHeader.set_ip(m_ip);
    //����ͷ��rpcHeader��С��Ϣ����ת�����ַ���������ʹ��ԭ��api :send���з�������(�����ոյ�service_name��method_name��Щ�����ڲ���protobuf��װ��
    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {

        controller->SetFailed("serialize rpc header error!");
        return;
    }
    //������Ҫ�ǽ�һЩ��Ϣת�������������Է�����淢��������
    std::string send_rpc_str;  // �����洢���շ��͵�����
    {
        // ����һ��StringOutputStream����д��send_rpc_str
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);

        // ��д��header�ĳ��ȣ��䳤���룩
        coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));

        // ����Ҫ�ֶ�д��header_size����Ϊ�����WriteVarint32�Ѿ�������header�ĳ�����Ϣ
        // Ȼ��д��rpc_header����
        coded_output.WriteString(rpc_header_str);
    }
    //��󣬽�����������ӵ�send_rpc_str����(���Ӧ�þ����������ˣ�
    //�����ڲ����л��Ѿ����
    //��������������ݷ�����
    send_rpc_str+= args_str;
    //�ͻ��˲��Ͻ������������з���
    ////send_rpc_str.size()ͨ�����ȷ���߽�
    while(-1==send(m_clientFd,send_rpc_str.c_str(),send_rpc_str.size(),0) ){
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        close(m_clientFd);
        m_clientFd = -1;
        std::string errMsg;
        if(!newConnect(m_ip.c_str(),m_port,&errMsg) ){
            controller->SetFailed("serialize rpc header error!");
            return ;
        }

    }
    //����rpc����Ӧֵ
    char recv_buf[1024]={0};
    int recv_size=0;
    if(-1 == (recv_size= recv(m_clientFd,recv_buf,1024,0) ) ){
        close(m_clientFd);
        m_clientFd=-1;
        char errtxt[512]={0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    //�����л�rpc���õ���Ӧ����,ͨ��grpc���ڲ��ķ����������л������ݷ����л�������Ӧ�Ŀͻ���

    //�����ܻὫ��Ӧ��response���ػ�ȥ���ɲ���ָ�����ʽ���ع�ȥ
    if(!response->ParseFromArray(recv_buf,recv_size) ){
        close(m_clientFd);
        m_clientFd=-1;
        char errtxt[1050]={0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
    close(m_clientFd);
    m_clientFd=-1;
}

bool Channel::newConnect(const char *ip, int port, std::string *errMsg) {
    int clientfd= socket(AF_INET,SOCK_STREAM,0);
    if(clientfd==-1){
        char errtext[512]={0};
        sprintf(errtext,"create socket fail errno :%d ",errno);

        m_clientFd=-1;
        *errMsg=errtext;
        return false;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port= htons(port);
    server_addr.sin_addr.s_addr= inet_addr(ip);

    //���ӷ����
    if(-1 == connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr) ) ){
        close(clientfd);
        char errtxt[512] = {0};
//        sprintf(errtxt, "connect fail! errno:%d", errno);
        sprintf(errtxt, "connect fail! errno %d, error: %s", errno, strerror(errno));
        m_clientFd = -1;
        *errMsg = errtxt;
        return false;
    }
    m_clientFd=clientfd;
    return true;
}

Channel::Channel(string ip, short port, bool connectNow,int type) {
    this->type=type;
    this->m_ip=ip;
    this->m_port=port;
    m_clientFd=-1;
    if(!connectNow){
        return ;
    }
    std::string errMsg;
    auto rt=newConnect(ip.c_str(),port,&errMsg);
    int tryCount =3;
    //�����ǰû�����ӻ�����Ҫ�������ӵĻ�����ô����������������3�Σ��������ʧ�ܣ���ô��û�취��
    while(!rt && tryCount--){
        rt=newConnect(ip.c_str(), port, &errMsg);
    }
}

