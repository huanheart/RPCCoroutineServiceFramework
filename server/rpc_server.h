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
    // ԭ����������ָ��ģ����淢��lock()->temp_map=service_map->unlock()���֣�ֱ��ȥ���ʶ�Ӧ��temp_map,�ǲ����ܴﵽ���������ģ���Ϊ
    //��ʱ����ָ��ͬһ����Դ�����ԣ�Ӧ�ø�Ϊ������ʽ!!

    //�ѷϳ���������ע������������������������ˣ������·����Ƿ���˶�ʱ��ע�����ķ�����Ϣ
    //������ֱ��ע������Ҳ���÷��ͣ����Ƕ�ʱ����Ӧʱ���������,��һ��ʱ���map
    //std::shared_ptr<std::unordered_map<std::string,sylar::Socket::ptr> > socket_map;
    std::unordered_map<std::string,std::time_t >  time_map;

public:
    void handleClient(std::shared_ptr<sylar::Socket> client) override;
    void InitHeartbeat();
private:
    void doHeartbeat();

    std::shared_ptr<Rpc_server> get_rpc_shared_ptr(){
        return std::static_pointer_cast<Rpc_server>(TcpServer::shared_from_this() ) ;   //��ʽ˵��������ģ����ѯ
    }
private:
    sylar::IOManager * m_ioManager=nullptr;
    const int thread_num=5;
    const bool use_caller=false;
};
