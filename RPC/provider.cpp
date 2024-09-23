//
// Created by lkt on 9/19/24.
//

#include "provider.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>


using namespace std;

provider::provider() {
    reset();
}

void  provider::reset() {
    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
}


bool provider::read_once() {
    int bytes_read=0;
    bytes_read=m_client->recv(m_read_buf,READ_BUFFER_SIZE,0);
    if(bytes_read==-1){
        if(!(errno==EAGAIN||errno==EWOULDBLOCK) ) {
            return false;
        }
    }else if(bytes_read==0){
        return false;
    }
    return true;
}

bool provider::OnMessage() {
    size_t len= strlen(m_read_buf);
    recv_buf=string(m_read_buf,len);
    google::protobuf::io::ArrayInputStream array_input(recv_buf.data(),recv_buf.size() );
    google::protobuf::io::CodedInputStream coded_input(&array_input);
    uint32_t header_size{};

    coded_input.ReadVarint32(&header_size);  // 这里先设置读取的类型，是int32类型的
    //// 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息

    //// 解析header_size（防止读取多余的数据，即防止读到请求体的东西什么的，正确保证读取到头的信息,而不是请求体）
    google::protobuf::io::CodedInputStream::Limit msg_limit =coded_input.PushLimit(header_size);

    ////反序列化
    coded_input.ReadString(&rpc_header_str,header_size);

    coded_input.PopLimit(msg_limit);
    uint32_t args_size{};
    /////主要将头部的一些信息拿出来，并且从stting类型变成RpcHeader类型
    if(rpcHeader.ParseFromString(rpc_header_str) ) {
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
        type=rpcHeader.message_type();
        ip=rpcHeader.ip();
        port=rpcHeader.port();

    }else{
        //// 数据头反序列化失败
        return false;
    }
    bool read_args_success = coded_input.ReadString(&args_str, args_size);
    if(!read_args_success){
        return false;
    }


    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end()) {
        std::cout << "service： " << service_name << " is not exist!" << std::endl;
        std::cout << "now server list is: ";
        for (auto item : m_serviceMap) {
            std::cout << item.first << " ";
        }
        std::cout << std::endl;
        return false;
    }
    //找对应服务中某一个具体的方法
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end()) {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return false;
    }
    google::protobuf::Service *service = it->second.m_service;       // 获取service对象  new UserService
    const google::protobuf::MethodDescriptor *method = mit->second;  // 获取method对象  Login

    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str)) {
        std::cout << "request parse error, content:" << args_str << std::endl;
        return false;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    //////！！！！！重点这个NewCallback是在CallMethod之后发生的，固然response是有数据被序列化的

    google::protobuf::Closure * done=google::protobuf::NewCallback<provider,sylar::Socket::ptr,google::protobuf::Message *>(
            this,&provider::SendRpcResponse,m_client,response);
    service->CallMethod(method, nullptr,request,response,done);
    return true;
}


void provider::SendRpcResponse(sylar::Socket::ptr conn, google::protobuf::Message * response) {
    std::string response_str;
    if(response->SerializeToString(&response_str) ){
        conn->send(response_str.c_str(),response_str.size(),0);
    }else {
        std::cout<<"serialize response_str error! "<<std::endl;
    }

}

void provider::NotifyService(google::protobuf::Service * service) {
    ServiceInfo service_info; //用于接收服务的一些信息的类

    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor * pserviceDesc =service->GetDescriptor();

    //获取服务的名字
    std::string service_name=pserviceDesc->name();
    //获取服务对象service的方法的数量,即这个服务有多少个接口可以给我们调用?
    int method_cnt=pserviceDesc->method_count();

    std::cout << "service_name:  " << service_name << std::endl;
    for(int i=0;i<method_cnt;++i){
        //获取每一个具体的接口信息，然后存到哈希表中
        const google::protobuf::MethodDescriptor * pmethodDesc = pserviceDesc->method(i);
        std::string method_name=pmethodDesc->name();
        service_info.m_methodMap.insert( {method_name,pmethodDesc} );
    }
    service_info.m_service=service;
    m_serviceMap.insert( {service_name,service_info} );
}

void provider::handleClient(sylar::Socket::ptr client){
    sylar::set_hook_enable(true);

    int client_socket=client->getSocket();
    this->m_client=client;
    if(!client->isValid()){
        client->close();
        return ;
    }
    while(client->isConnected()){
        if(read_once()==false){
            goto end;
        }

        if(OnMessage()==false){
            goto end;
        }
        reset();
    }

end:
    client->close();
}
