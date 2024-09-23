#include"channel.h"
#include<arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include<muduo/base/Logging.h>

//下面是搬运util的
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
//上面搬运

#include"rpcheader.pb.h"
#include"control.h"
#include"../util/config.h"

using namespace std;

//(重点：由于rpc的发送protobuf的数据需要有边界，防止tcp的粘包等问题，固然此时前面的部分是自己设置的头（用来标识边界），后面的部分是体，是真正的内容
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
    ////获取当前服务的名称，以及服务对应的某个方法名
    ////得到一个服务描述符知道一些信息
    const google::protobuf::ServiceDescriptor * sd=method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();
    //获取参数的序列化字符串长度
    uint32_t args_size{};
    std::string args_str;
    //将request其序列化成字符串（这个转化成字符串的意思是转化成protobuf类型的string，并不是从protobuf转化成string）并储存到args_str中，方便获取长度
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
    //构建头部rpcHeader大小信息并且转化成字符串，方便使用原生api :send进行发送数据(即将刚刚的service_name，method_name这些进行内部的protobuf封装）
    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {

        controller->SetFailed("serialize rpc header error!");
        return;
    }
    //这里主要是将一些信息转换成数据流，以方便后面发送数据流
    std::string send_rpc_str;  // 用来存储最终发送的数据
    {
        // 创建一个StringOutputStream用于写入send_rpc_str
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);

        // 先写入header的长度（变长编码）
        coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));

        // 不需要手动写入header_size，因为上面的WriteVarint32已经包含了header的长度信息
        // 然后写入rpc_header本身
        coded_output.WriteString(rpc_header_str);
    }
    //最后，将请求参数附加到send_rpc_str后面(这个应该就是请求体了）
    //现在内部序列化已经完毕
    //将真正请求的数据放入体
    send_rpc_str+= args_str;
    //客户端不断进行重连并进行发送
    ////send_rpc_str.size()通过这个确定边界
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
    //接收rpc的响应值
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
    //反序列化rpc调用的响应数据,通过grpc中内部的方法，将序列化的数据反序列化给到对应的客户端

    //后面框架会将对应的response返回回去，由参数指针的形式返回过去
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

    //链接服务端
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
    //如果当前没有连接或者需要重新连接的话，那么就最多给它重新连接3次，如果还是失败，那么就没办法了
    while(!rt && tryCount--){
        rt=newConnect(ip.c_str(), port, &errMsg);
    }
}

