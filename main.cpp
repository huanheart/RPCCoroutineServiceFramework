#include<iostream>
#include<string>
#include <muduo/base/Logging.h>
#include<muduo/base/AsyncLogging.h>
#include"server/rpc_server.h"

#include <sys/resource.h> //设置虚拟内存的
using namespace std;

sylar::IOManager::ptr worker=nullptr;
off_t kRollSize = 500*1000*1000;
muduo::AsyncLogging * g_asyncLog=nullptr;
short Centerport=9906;

void asyncOutput(const char * msg,int len)
{
    g_asyncLog->append(msg,len);
}


void run()
{
    sylar::Address::ptr m_adress=sylar::Address::LookupAnyIPAddress("192.168.15.128:"+to_string(Centerport) );
    //创建rpc_server
    std::shared_ptr<Rpc_server> Rpcserver = std::make_shared<Rpc_server>();
    Rpcserver->InitHeartbeat();
    ////一定要在调取器创建之后之后new且用智能指针初始化，不能直接new
    /// 否则将会出错terminate called after throwing an instance of 'std::bad_weak_ptr'
    //启动bind函数，用于进行监听服务端socket
    while(!Rpcserver->bind(m_adress,false) ){
        sleep(1);
    }
    cout<<"Registration Center start!!!"<<endl;

    Rpcserver->start();
}

int main()
{
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
    worker.reset(new sylar::IOManager(5,false) );
    sylar::IOManager manager(1);
    manager.scheduleLock([=]() {
        run();
    });
    return 0;
}


