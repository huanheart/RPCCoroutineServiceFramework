
#include<iostream>
#include<string>
#include<mutex>
#include"../rpc.pb.h"
#include "../../RPC/channel.h"
#include "../../RPC/control.h"
#include"../../CoroutineLibrary/ioscheduler.h"
#include"../../util/address.h"
#include"../../RPC/rpcheader.pb.h"
#include"../../RPC/provider.h"

using namespace std;

string ip="192.168.15.128";
const char * Centerip="192.168.15.128";
short port=-1;       //��Ӧ����Ķ˿�
int clientfd=-1;
uint16_t Centerport=9906; //ע�����ĵĶ˿�
string Serviceip="";
string Serviceport="";

bool myConnect(){
    ////������Ҫ��ע�����Ľ�������
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
    return true;
}

bool GetIpAndPort()
{
    RPC::RpcHeader rpcHeader;
    string service_name="exampleService";
    string method_name="GetList";
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_message_type(0);
    rpcHeader.set_ip(ip);
    rpcHeader.set_port(port);
    rpcHeader.set_args_size(128);

    string serializedData;
    if(!rpcHeader.SerializeToString(&serializedData) ){
        return false;
    }

    uint32_t  data_size= htonl(serializedData.size());
    string buffer;
    buffer.append(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    buffer.append(serializedData);
    ////Ȼ�����ڿ�ʼ����
    send(clientfd,buffer.c_str(),buffer.size(),0);
    ////Ȼ���������
    char recv_buffer[2048];
    memset(recv_buffer,'\0',sizeof(recv_buffer) );
    int bytes_read=recv(clientfd,recv_buffer,sizeof(recv_buffer),0);
    string res=string(recv_buffer,bytes_read);
    if(res=="error"){
        return false;
    }
    ////��ʼ���Ҷ�Ӧip��ַ�Լ��˿ں�
    size_t pos=res.find(':');
    if(pos==string::npos){
        return false;
    }
    Serviceip=res.substr(0,pos);
    Serviceport=res.substr(pos+1,res.size()-pos);
    cout<<"get success "<<Serviceip<<' '<<Serviceport<<endl;
    return true;
}

////��Ҫ��һ���߳�ȥ��������ע�����ķ��͹��������ݣ�ģ�����ͻ�����һ���������������
////����example��ֻ��ģ�⣬����д�Ĳ���������ţ���

int main(){
    ////��Ҫ�ȴ�ע��������ȡ��Ӧ�Ķ˿��Լ�ip��Ϣ
    myConnect();
    ////ÿ5�����һ�η���
    while(true){
        ////��ʱ��Ҫѯ���Ƿ����������
        if(Serviceport=="") {
            bool word = GetIpAndPort();
            ////�������������ɾ���ˣ��ȴ�������һ��ע��
            if (Serviceport == "" || word == false) {
                cout << "get port failed !!! " << endl;
            }
            sleep(5);
            continue;
        }
        example::ServiceRpc_Stub stub(new Channel(Serviceip,stoi(Serviceport),true,0) );
        Control controller;
        example::GetListRequest request;
        request.set_userid(1000);
        example::GetListResponse response;
        stub.GetList(&controller, &request, &response, nullptr);
        ////�����ִ����ʱ�򣬣����������ʧ���ͻ�����Ҫ����Ӧ����
        ////����ģ�⽫��Ӧ�ͻ��˻������������Ȼ����һ��Զ��rpc���õ�ʱ��Ҫ������ȡrpc����˵�ip��ַ�Լ��˿���Ϣ
        //// ��zookeeper��Щ�м����Ӧ��������֪ͨ�ͻ���ȡ������ģ��������ÿͻ��˵�����һ��rpc���ã�����ʧ�ܲŽ���ȡ������
        //// Ҫ�ﵽ����Ч����ע�����Ŀ���ά��һ��map������Ŀǰֻ�뵽�˲����ŵĽⷨ���������ܻ��
        if (controller.Failed()) {
            cout<<"request failed ! reason "<<controller.ErrorText()<<endl;
            Serviceip="";
            Serviceport="";
            sleep(5);
            continue;
        }
        if (0 == response.result().errcode()) {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.val_size();
            for (int i = 0; i < size; i++) {
                std::cout << "index:" << (i + 1) << " name:" << response.val(i) << std::endl;
            }
        } else {
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
        sleep(5);
    }
    return 0;
}


