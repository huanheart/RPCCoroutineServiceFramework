//
// Created by lkt on 9/19/24.
//

#ifndef TEST_RPC_PROVIDER_H
#define TEST_RPC_PROVIDER_H

#include"../server/tcp_server.h"
#include"../util/socket.h"
#include"../util/config.h"
#include"rpcheader.pb.h"

#include <functional>
#include <string>
#include <unordered_map>
#include "google/protobuf/service.h"


class provider :public sylar::TcpServer {

public:
    provider();
    void NotifyService(google::protobuf::Service * service);
    bool read_once();
    bool OnMessage();
    void SendRpcResponse(sylar::Socket::ptr conn, google::protobuf::Message * response);
public:
    void handleClient(std::shared_ptr<sylar::Socket> client) override;
    bool do_echo();
private:
    void reset();

private:
    static const int READ_BUFFER_SIZE=2048;
    char m_read_buf[READ_BUFFER_SIZE];
    uint32_t port{};
    uint32_t type{};
    uint32_t args_size{};
    std::string recv_buf;

    std::string rpc_header_str;
    std::string service_name;
    std::string method_name;
    std::string ip;
    std::string args_str;

    sylar::Socket::ptr m_client;
    RPC::RpcHeader rpcHeader;

    struct ServiceInfo {
        google::protobuf::Service *m_service;                                                     // 保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;  // 保存服务方法
    };
    // 存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

};


#endif //TEST_RPC_PROVIDER_H
