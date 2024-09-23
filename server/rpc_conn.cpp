#include "rpc_conn.h"


#include<fstream>
#include<mutex>
#include<muduo/base/Logging.h>
#include <unordered_map>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <ctime>
#include"../CoroutineLibrary/ioscheduler.h"
#include"../CoroutineLibrary/hook.h"
#include<google/protobuf/io/coded_stream.h>

using namespace std;


// �洢��Ӧ�ķ���˵�ip��ַ�Լ��˿ں�
//�����map��Ҫ������,���ҽ���Ӧ����˵�ip��ַ��Щ����һ����¼
//�������Ҫ��ʱ����ע��
void Rpc_conn::NotifyService() {
    string str=service_name+method_name;
    string val=ip+":"+to_string(static_cast<int>(port) );
    rpcServer->mtx.lock();
    rpcServer->service_map[str]=val;
    rpcServer->time_map[str]=std::time(nullptr); //����������ע�ᣬ�����˶�ʱ���µ�����
    rpcServer->mtx.unlock();
}

//��߽�ԭʼ���ݽ���ת������Ӧ�ķ���˵�tcpserver��
//ʹ��send��recv�������в���,�ڲ���������read_once����
//�ͻ��˹������в�ѯ��
void Rpc_conn::CallingMethods() {
    string str=service_name+method_name;
    string back="error";
    rpcServer->mtx.lock();
    bool decide=rpcServer->service_map.count(str);
    if(decide){
        back=rpcServer->service_map[str];
    }
    rpcServer->mtx.unlock();

    if(send(m_client->getSocket(),back.c_str(),back.size(),0) <0){
        LOG_INFO<<"Registration Center send failed ! ";
    }

}


void Rpc_conn::init(sylar::Socket::ptr &client,std::shared_ptr<Rpc_server> main_rpcServer) {
    if(rpcServer.get()==nullptr){
        rpcServer=main_rpcServer;
    }
    this->m_client=client;
    reset();
}

void Rpc_conn::reset() {
    memset(m_read_buf,'\0',sizeof(recv_buf));
}

bool Rpc_conn::read_once() {
    int bytes_read=0;
    bytes_read=m_client->recv(m_read_buf,READ_BUFFER_SIZE,0);
    if(bytes_read==-1){
        if( !(errno==EAGAIN||errno==EWOULDBLOCK) ) {
            return false;
        }
    }else if(bytes_read==0){
        return false;
    }
    return true;
}


bool Rpc_conn::process() {
    uint32_t data_size;
    memcpy(&data_size,m_read_buf,sizeof(data_size) );
    data_size= ntohl(data_size);
    //ȷ�����յ����ֽ����㹻����������Ϣ�Լ�ʵ������
//    if(m_read_idx <sizeof(data_size)+data_size){
//        return false;
//    }
    ////�ӻ�������ȡ���л���RpcHeader ����
    string serializedData(m_read_buf+sizeof(data_size),data_size);
    ////�����л�RpcHeader
    RPC::RpcHeader rpcHeader;
    if(!rpcHeader.ParseFromString(serializedData) ){
        return false;
    }
    service_name=rpcHeader.service_name();
    method_name=rpcHeader.method_name();
    ip=rpcHeader.ip();
    port=rpcHeader.port();
    type=rpcHeader.message_type();
    args_size=rpcHeader.args_size();
    ////0��ʾ�ͻ������󷽷���1��ʾ����˽��з�������
    if(type==1){
        NotifyService();
    }else {
        CallingMethods();
    }
    reset();
    return true;

}




