#ifndef RPCCONNECTION_H
#define RPCCONNECTION_H
#include <unistd.h> //��fork������Щ����
#include <signal.h>
#include <sys/types.h> //�����������pid_t��size_t��ssize_t�����͵Ķ��壬��Щ������ϵͳ����о������ڱ�ʾ����ID����С���з��Ŵ�С�ȡ�
#include <sys/epoll.h>
#include <fcntl.h> //���ڶ��Ѵ򿪵��ļ����������и��ֿ��Ʋ������������ļ���������״̬��־��������I/O�ȡ�
#include <sys/socket.h>
#include <netinet/in.h> //Internet��ַ�����ؽṹ��ͷ��ų���,��������Ľṹ����sockaddr_in�����ڱ�ʾIPv4��ַ�Ͷ˿ںš�������������
#include <arpa/inet.h>  //������һЩ��IPv4��ַת����صĺ���
#include <assert.h>     //��ͷ�ļ�������һ����assert()�������ڳ����в������
#include <sys/stat.h>   //���ڻ�ȡ�ļ���״̬��Ϣ�����ļ��Ĵ�С������Ȩ�޵ȡ�
#include<cstring>
#include<string>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h> //��ͷ�ļ�������һЩ����̵ȴ���صĺ����ͺ꣬������õĺ�����waitpid()�����ڵȴ�ָ�����ӽ���״̬�����仯
#include <sys/uio.h>
// ��ͷ�ļ�������һЩ��I/O����������صĺ����ͽṹ�塣
// ������õĽṹ����iovec����������һ��ϵͳ�����д������������ڴ����������
// ��������������ʹ��readv()��writev()���������з�ɢ��ȡ�;ۼ�д�롣
#include <map>

#include <google/protobuf/descriptor.h>
//#include<google/protobuf/io/array_input_stream.h>
#include<google/protobuf/io/coded_stream.h>

#include "../RPC/rpcheader.pb.h"
#include "google/protobuf/service.h"
#include"../util/socket.h"

#include"rpc_server.h"

class Rpc_server; //ǰ������

class Rpc_conn
{
    friend class Rpc_server;
    std::shared_ptr<Rpc_server> rpcServer;    //����ÿ�һ������ָ��
public:
    static const int READ_BUFFER_SIZE=2048;

public:
    bool read_once();
    bool process();
    void init(sylar::Socket::ptr &client,std::shared_ptr<Rpc_server> main_rpcServer);
public:
    //���Ҫ�����ڴ���룬��ֹ�ڲ�����ʹ�ÿռ����
    char m_read_buf[READ_BUFFER_SIZE];
    uint32_t args_size{};
    uint32_t port{};
    uint32_t type{};
    std::string recv_buf;

    std::string rpc_header_str;
    std::string service_name;
    std::string method_name;
    std::string ip;

    sylar::Socket::ptr m_client;
    RPC::RpcHeader rpcHeader;



private:
    void CallingMethods();
    void NotifyService();
    void reset();
//    bool newConnect(const char *ip,uint16_t port,string *errMsg);
};





#endif