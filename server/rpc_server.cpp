#include"rpc_server.h"
#include"../CoroutineLibrary/hook.h"
#include"rpc_conn.h"


Rpc_conn * users;

////InitHeartbeat多开一个这个函数，而不是在构造函数做事，防止this没有初始化完毕
void Rpc_server::InitHeartbeat() {
//    m_ioManager.reset(new sylar::IOManager(1,use_caller) );       //绝对不能嵌套（原因目前我还搞不清楚）
    m_ioManager=sylar::IOManager::GetThis();
    m_ioManager->scheduleLock(std::bind(&Rpc_server::doHeartbeat,get_rpc_shared_ptr() ) ); //成员函数会默认一个this参数，固然我们传一个this进去
}

void Rpc_server::doHeartbeat() {

    while(true){
        //每隔30s查询一次,如果map[str]60s没有进行心跳，那么就
        sleep(30);
        std::time_t nowtime=std::time(nullptr);
        mtx.lock();
        std::unordered_map<std::string,std::time_t > temp_map=time_map;
        mtx.unlock();
        for(auto &m : temp_map) {
            if (m.second + 60 < nowtime) {//说明超时了
                //删除对应的time_map以及service_map的内容
                mtx.lock();
                std::cout<<"Time is out!!!!      "<<m.first<<std::endl;
                time_map.erase(m.first);
                service_map.erase(m.first);
                mtx.unlock();
            }
        }

    }

}

Rpc_server::Rpc_server(){
    sylar::set_hook_enable(true);
    users=new Rpc_conn[MAX_FD];
}
Rpc_server::~Rpc_server(){
    if(users)
        delete users;
}

void Rpc_server::handleClient(sylar::Socket::ptr client){
    //这个可能需要注意一下，还需要判断是否能够在注册中心中使用hook技术
//    cout<<"handleClient start "<<endl;
    int client_socket=client->getSocket();
    if(!client->isValid()){
        client->close();
        return ;
    }
    users[client_socket].init(client,get_rpc_shared_ptr() ); //这里不能直接将this传递给对应shared_ptr指针，必须继承于std::enable_shared_from_this<Rpc_server>,然后使用当前shared_from_this函数
    while(client->isConnected()){
        if(users[client_socket].read_once()==false){
            goto end;
        }
            //查看他发送过来的信息
        LOG_INFO <<users[client_socket].m_read_buf;
        //现在开始处理（即需要判断是服务端发送过来的更新还是客户端的调用），这里固然需要有自己的protobuf格式,即头部得自定义一个标识
        if(users[client_socket].process()==false){
            goto end;
        }
    }

end:

    client->close();

}
