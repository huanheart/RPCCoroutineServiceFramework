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

////����serverֻ�ṩ���û������ܶ��������Խ������󣬵��ǲ�û�������������ӳ�䣨��ע�����ĵ�Ψһӳ�䣩�������������Ȼ�䲢����������ʧЧ
////���޸ĳɲ�����ȫ��ֻ����rpc_conn��rpc_server����һ�¼���

using namespace std;

sylar::IOManager::ptr worker=nullptr;
sylar::IOManager* sendDataWorker=nullptr;
muduo::AsyncLogging * g_asyncLog=nullptr;
string ip="192.168.15.128";
int port=9910;
int clientfd=-1;
const char * Centerip="192.168.15.128";
uint16_t Centerport=9906; //ע�����ĵĶ˿�
off_t kRollSize = 500*1000*1000;


void asyncOutput(const char * msg,int len)
{
    g_asyncLog->append(msg,len);
}

//����������ʱ��������Ҫ��ע�ᣬȻ���ٿ�ʼ��Ӧ�ļ���
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
    //done->Run����֪ͨ RPC ��� RPC �������,��ʱ�Ϳ��Դ���provider�еĻص��ˣ���ʱ��������ת����protobuf���ݲ�ͨ��ά����send������Ӧ��ȥ
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
    //���ͬ������
    if(send(clientfd,buffer.c_str(),buffer.size(),0) <0){
        close(clientfd);
        clientfd = -1;
        return false;
    }
    return true;
}

//�����Լ���ip��ַ�˿ں���Ϣ����protobuf���з���
bool sendRegistrationCenter(){
//    LOG_INFO<<"aaa";
    //������Ҫ��ע�����Ľ�������,��ʱ����ǿͻ���
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
    //Ȼ����߿�ʼ���Ͷ�Ӧprotobuf����
    SendData();
//    close(clientfd);     //Ŀǰ������ɾ�������潫�ͻ������������ʱ�򣬾Ͳ���ɾ���ˣ�
    return true;
}

void doHeart()
{
    while(true){
        //ÿ��15s��ע�����Ľ���һ�η��ͷ���
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
    //����Ҫע�ᵽ��Ӧ����ע��������
    //��ȡ�����ļ���֪����Ӧ��λ��
    while(!sendRegistrationCenter()){
        sleep(1);
    }
    sendDataWorker=sylar::IOManager::GetThis();
    sendDataWorker->scheduleLock(std::bind(&doHeart) );
    while(!server->bind(m_address,false) ){
        sleep(1);
    }
    server->NotifyService(new exampleService());
    ////�ص㣡�������������ڿ��ǵ��������ƵĽ��գ���ô����ֱ��ͨ��ע�����Ŀ�����Э��
    ////��ʱ��ȥ���ö�Ӧ�ķ����������ˣ����Կ���һ������ĵ�����ȥ�������Э��ȥ����
    server->start();
}

int main() {
    {
        // �������������ڴ�Ϊ2GB
        size_t kOneGB = 1000*1024*1024;
        rlimit rl = { 2*kOneGB, 2*kOneGB };
        setrlimit(RLIMIT_AS, &rl);
    }
    char name[256]={'\0'};
    muduo::AsyncLogging log(::basename(name), kRollSize);
    log.start();
    g_asyncLog = &log;
    muduo::Logger::setOutput(asyncOutput);

    //����˵�Ĭ�϶˿ں�
    worker.reset(new sylar::IOManager(3,false));
    sylar::IOManager manager(1);
    manager.scheduleLock([=](){
        run();
    });
    return 0;
}