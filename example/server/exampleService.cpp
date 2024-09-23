//
// Created by lkt on 9/19/24.
//
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include<vector>
#include <muduo/base/Logging.h>
#include<muduo/base/AsyncLogging.h>
#include <sys/resource.h>
#include"../rpc.pb.h"
#include"../../RPC/provider.h"
#include"../../RPC/rpcheader.pb.h"
#include"../../CoroutineLibrary/ioscheduler.h"
#include"../../CoroutineLibrary/hook.h"
#include "../../util/address.h"

////这里server只提供单用户，尽管多个服务可以进行请求，但是并没有针对其做数组映射（即注册中心的唯一映射）或加锁操作，固然其并发请求服务会失效
////若修改成并发安全，只需像rpc_conn与rpc_server处理一致即可

using namespace std;

sylar::IOManager::ptr worker=nullptr;
sylar::IOManager* sendDataWorker=nullptr;
muduo::AsyncLogging * g_asyncLog=nullptr;
string ip="192.168.15.128";
int port=9910;
int clientfd=-1;
const char * Centerip="192.168.15.128";
uint16_t Centerport=9906; //注册中心的端口
off_t kRollSize = 500*1000*1000;


void asyncOutput(const char * msg,int len)
{
    g_asyncLog->append(msg,len);
}

//创建这个类的时候，我们需要先注册，然后再开始对应的监听
class exampleService :public example::ServiceRpc{
private:
    vector<string> v;
public:
    void PushData(){
        v.push_back("welcome to huanheart project");
        v.push_back("Here is my blog link ");
        v.push_back("https://github.com/huanheart");
    }
    void GetList(::google::protobuf::RpcController *controller,const ::example::GetListRequest *request,
                ::example::GetListResponse * response,::google::protobuf::Closure *done) override;

};



void exampleService::GetList(::google::protobuf::RpcController *controller,const ::example::GetListRequest *request,
                             ::example::GetListResponse * response,::google::protobuf::Closure *done) {
    uint32_t  userid=request->userid();
    PushData();
    response->mutable_result()->set_errcode(0);
    response->mutable_result()->set_errmsg("");
    for(int i=0;i<v.size();i++){
        string *p=response->add_val();
        *p=v[i];
    }
    v.clear();
    //done->Run函数通知 RPC 框架 RPC 调用完成,此时就可以触发provider中的回调了，此时将其数据转化成protobuf数据并通过维护的send方法响应回去
    done->Run();
}


bool SendData()
{
    string service_name="exampleService";
    string method_name="GetList";
    RPC::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_message_type(1);
    rpcHeader.set_ip(ip);
    rpcHeader.set_port(port);
    rpcHeader.set_args_size(128);

    string serializedData;
    if(!rpcHeader.SerializeToString(&serializedData) ){
        close(clientfd);
        clientfd = -1;
        return false;
    }
    uint32_t  data_size= htonl(serializedData.size());
    string buffer;
    buffer.append(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    buffer.append(serializedData);
    //这边同步发送
    if(send(clientfd,buffer.c_str(),buffer.size(),0) <0){
        close(clientfd);
        clientfd = -1;
        return false;
    }
    return true;
}

//发送自己的ip地址端口号信息，用protobuf进行发送
bool sendRegistrationCenter(){
//    LOG_INFO<<"aaa";
    //首先需要和注册中心建立连接,此时这边是客户端
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
    //然后这边开始发送对应protobuf数据
    SendData();
//    close(clientfd);     //目前这里先删除，后面将客户端启动服务的时候，就不能删除了，
    return true;
}

void doHeart()
{
    while(true){
        //每个15s给注册中心进行一次发送服务
        sleep(15);
        if(clientfd==-1){
            std::cout<<"clientfd==-1 !!!! so doHeart Failed"<<std::endl;
            exit(1);
        }
        if(SendData()==false){
            std::cout<<"SendData==false !!!! so doHeart Failed"<<std::endl;
            exit(1);
        }

    }
}





void run()
{

    sylar::Address::ptr m_address=sylar::Address::LookupAnyIPAddress(ip+":"+to_string(port) );
    std::shared_ptr<provider> server=std::make_shared<provider>();
    //还需要注册到对应服务到注册中心上
    //读取配置文件，知道对应的位置
    while(!sendRegistrationCenter()){
        sleep(1);
    }
    sendDataWorker=sylar::IOManager::GetThis();
    sendDataWorker->scheduleLock(std::bind(&doHeart) );
    while(!server->bind(m_address,false) ){
        sleep(1);
    }
    server->NotifyService(new exampleService());
    ////重点！！！！！！由于考虑到心跳机制的接收，那么我们直接通过注册中心开几个协程
    ////定时地去调用对应的服务给到服务端，可以开辟一个额外的调度器去创建这个协程去工作
    server->start();
}

int main() {
    {
        // 设置最大的虚拟内存为2GB
        size_t kOneGB = 1000*1024*1024;
        rlimit rl = { 2*kOneGB, 2*kOneGB };
        setrlimit(RLIMIT_AS, &rl);
    }
    char name[256]={'\0'};
    muduo::AsyncLogging log(::basename(name), kRollSize);
    log.start();
    g_asyncLog = &log;
    muduo::Logger::setOutput(asyncOutput);

    //服务端的默认端口号
    worker.reset(new sylar::IOManager(3,false));
    sylar::IOManager manager(1);
    manager.scheduleLock([=](){
        run();
    });
    return 0;
}