
#include<iostream>
#include<string>
#include<mutex>
#include"../rpc.pb.h"
#include "../../RPC/channel.h"
#include "../../RPC/control.h"
#include"../../CoroutineLibrary/ioscheduler.h"
#include"../../util/address.h"
#include"../../RPC/rpcheader.pb.h"
#include"../../RPC/provider.h"

using namespace std;

string ip="192.168.15.128";
const char * Centerip="192.168.15.128";
short port=-1;       //对应服务的端口
int clientfd=-1;
uint16_t Centerport=9906; //注册中心的端口
string Serviceip="";
string Serviceport="";

bool myConnect(){
    ////首先需要和注册中心建立连接
    clientfd= socket(AF_INET,SOCK_STREAM,0);
    if(clientfd==-1){
        return false;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Centerport);
    server_addr.sin_addr.s_addr = inet_addr(Centerip);

    if(-1==connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr) ) ){
        close(clientfd);
        clientfd = -1;
        return false;
    }
    return true;
}

bool GetIpAndPort()
{
    RPC::RpcHeader rpcHeader;
    string service_name="exampleService";
    string method_name="GetList";
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_message_type(0);
    rpcHeader.set_ip(ip);
    rpcHeader.set_port(port);
    rpcHeader.set_args_size(128);

    string serializedData;
    if(!rpcHeader.SerializeToString(&serializedData) ){
        return false;
    }

    uint32_t  data_size= htonl(serializedData.size());
    string buffer;
    buffer.append(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    buffer.append(serializedData);
    ////然后现在开始请求
    send(clientfd,buffer.c_str(),buffer.size(),0);
    ////然后接收数据
    char recv_buffer[2048];
    memset(recv_buffer,'\0',sizeof(recv_buffer) );
    int bytes_read=recv(clientfd,recv_buffer,sizeof(recv_buffer),0);
    string res=string(recv_buffer,bytes_read);
    if(res=="error"){
        return false;
    }
    ////开始查找对应ip地址以及端口号
    size_t pos=res.find(':');
    if(pos==string::npos){
        return false;
    }
    Serviceip=res.substr(0,pos);
    Serviceport=res.substr(pos+1,res.size()-pos);
    cout<<"get success "<<Serviceip<<' '<<Serviceport<<endl;
    return true;
}

////需要开一个线程去阻塞接收注册中心发送过来的数据（模拟假设客户端是一个服务器的情况）
////由于example中只是模拟，所以写的并不会很优雅！！

int main(){
    ////需要先从注册中心拿取对应的端口以及ip信息
    myConnect();
    ////每5秒调用一次方法
    while(true){
        ////此时需要询问是否有这个服务
        if(Serviceport=="") {
            bool word = GetIpAndPort();
            ////可能是这个服务被删除了，等待它进行一个注册
            if (Serviceport == "" || word == false) {
                cout << "get port failed !!! " << endl;
            }
            sleep(5);
            continue;
        }
        example::ServiceRpc_Stub stub(new Channel(Serviceip,stoi(Serviceport),true,0) );
        Control controller;
        example::GetListRequest request;
        request.set_userid(1000);
        example::GetListResponse response;
        stub.GetList(&controller, &request, &response, nullptr);
        ////当出现错误的时候，（比如服务消失）客户端需要做对应处理
        ////这里模拟将对应客户端缓存进行消除，然后下一次远程rpc调用的时候，要继续获取rpc服务端的ip地址以及端口信息
        //// 像zookeeper这些中间件，应该是主动通知客户端取消缓存的，而不是让客户端调用了一次rpc调用，发现失败才进行取消缓存
        //// 要达到这样效果，注册中心可以维护一个map，但是目前只想到了不优雅的解法，后续可能会改
        if (controller.Failed()) {
            cout<<"request failed ! reason "<<controller.ErrorText()<<endl;
            Serviceip="";
            Serviceport="";
            sleep(5);
            continue;
        }
        if (0 == response.result().errcode()) {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.val_size();
            for (int i = 0; i < size; i++) {
                std::cout << "index:" << (i + 1) << " name:" << response.val(i) << std::endl;
            }
        } else {
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
        sleep(5);
    }
    return 0;
}


