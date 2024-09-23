#ifndef RPCCONNECTION_H
#define RPCCONNECTION_H
#include <unistd.h> //有fork（）这些函数
#include <signal.h>
#include <sys/types.h> //其中最常见的是pid_t、size_t、ssize_t等类型的定义，这些类型在系统编程中经常用于表示进程ID、大小和有符号大小等。
#include <sys/epoll.h>
#include <fcntl.h> //用于对已打开的文件描述符进行各种控制操作，如设置文件描述符的状态标志、非阻塞I/O等。
#include <sys/socket.h>
#include <netinet/in.h> //Internet地址族的相关结构体和符号常量,其中最常见的结构体是sockaddr_in，用于表示IPv4地址和端口号。用于网络编程中
#include <arpa/inet.h>  //声明了一些与IPv4地址转换相关的函数
#include <assert.h>     //该头文件声明了一个宏assert()，用于在程序中插入断言
#include <sys/stat.h>   //用于获取文件的状态信息，如文件的大小、访问权限等。
#include<cstring>
#include<string>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h> //该头文件声明了一些与进程等待相关的函数和宏，其中最常用的函数是waitpid()，用于等待指定的子进程状态发生变化
#include <sys/uio.h>
// 该头文件声明了一些与I/O向量操作相关的函数和结构体。
// 其中最常用的结构体是iovec，它用于在一次系统调用中传输多个非连续内存区域的数据
// 例如在网络编程中使用readv()和writev()函数来进行分散读取和聚集写入。
#include <map>

#include <google/protobuf/descriptor.h>
//#include<google/protobuf/io/array_input_stream.h>
#include<google/protobuf/io/coded_stream.h>

#include "../RPC/rpcheader.pb.h"
#include "google/protobuf/service.h"
#include"../util/socket.h"

#include"rpc_server.h"

class Rpc_server; //前向声明

class Rpc_conn
{
    friend class Rpc_server;
    std::shared_ptr<Rpc_server> rpcServer;    //这里得开一个智能指针
public:
    static const int READ_BUFFER_SIZE=2048;

public:
    bool read_once();
    bool process();
    void init(sylar::Socket::ptr &client,std::shared_ptr<Rpc_server> main_rpcServer);
public:
    //这个要满足内存对齐，防止内部对齐使得空间过大
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