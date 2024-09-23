#pragma  once

#include <google/protobuf/descriptor.h>
#include "google/protobuf/service.h"

#include <functional>
#include<string>
#include<mutex>
#include<unordered_map>
#include <muduo/base/Logging.h>
#include<muduo/base/AsyncLogging.h>
#include"../util/socket.h"
#include "tcp_server.h"
#include"../CoroutineLibrary/ioscheduler.h"
#include<ctime>

using namespace std;

const int MAX_FD=65536;

class Rpc_server :public sylar::TcpServer
{
public:
    Rpc_server();
    ~Rpc_server();
    std::mutex mtx;
    std::unordered_map<std::string,std::string> service_map;
    //    std::shared_ptr<std::unordered_map<std::string,std::string> > service_map;
    // 原本想用智能指针的，后面发现lock()->temp_map=service_map->unlock()这种，直接去访问对应的temp_map,是并不能达到互不关联的，因为
    //此时依旧指向同一个资源，所以，应该改为拷贝形式!!

    //已废除，本想着注册中心主动发送心跳给服务端，查了下发现是服务端定时给注册中心发送信息
    //这样子直接注册中心也不用发送，但是定时检查对应时间戳即可了,开一个时间戳map
    //std::shared_ptr<std::unordered_map<std::string,sylar::Socket::ptr> > socket_map;
    std::unordered_map<std::string,std::time_t >  time_map;

public:
    void handleClient(std::shared_ptr<sylar::Socket> client) override;
    void InitHeartbeat();
private:
    void doHeartbeat();

    std::shared_ptr<Rpc_server> get_rpc_shared_ptr(){
        return std::static_pointer_cast<Rpc_server>(TcpServer::shared_from_this() ) ;   //显式说明，避免模糊查询
    }
private:
    sylar::IOManager * m_ioManager=nullptr;
    const int thread_num=5;
    const bool use_caller=false;
};
