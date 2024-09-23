#include"rpc_server.h"
#include"../CoroutineLibrary/hook.h"
#include"rpc_conn.h"


Rpc_conn * users;

////InitHeartbeat�࿪һ������������������ڹ��캯�����£���ֹthisû�г�ʼ�����
void Rpc_server::InitHeartbeat() {
//    m_ioManager.reset(new sylar::IOManager(1,use_caller) );       //���Բ���Ƕ�ף�ԭ��Ŀǰ�һ��㲻�����
    m_ioManager=sylar::IOManager::GetThis();
    m_ioManager->scheduleLock(std::bind(&Rpc_server::doHeartbeat,get_rpc_shared_ptr() ) ); //��Ա������Ĭ��һ��this��������Ȼ���Ǵ�һ��this��ȥ
}

void Rpc_server::doHeartbeat() {

    while(true){
        //ÿ��30s��ѯһ��,���map[str]60sû�н�����������ô��
        sleep(30);
        std::time_t nowtime=std::time(nullptr);
        mtx.lock();
        std::unordered_map<std::string,std::time_t > temp_map=time_map;
        mtx.unlock();
        for(auto &m : temp_map) {
            if (m.second + 60 < nowtime) {//˵����ʱ��
                //ɾ����Ӧ��time_map�Լ�service_map������
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
    //���������Ҫע��һ�£�����Ҫ�ж��Ƿ��ܹ���ע��������ʹ��hook����
//    cout<<"handleClient start "<<endl;
    int client_socket=client->getSocket();
    if(!client->isValid()){
        client->close();
        return ;
    }
    users[client_socket].init(client,get_rpc_shared_ptr() ); //���ﲻ��ֱ�ӽ�this���ݸ���Ӧshared_ptrָ�룬����̳���std::enable_shared_from_this<Rpc_server>,Ȼ��ʹ�õ�ǰshared_from_this����
    while(client->isConnected()){
        if(users[client_socket].read_once()==false){
            goto end;
        }
            //�鿴�����͹�������Ϣ
        LOG_INFO <<users[client_socket].m_read_buf;
        //���ڿ�ʼ��������Ҫ�ж��Ƿ���˷��͹����ĸ��»��ǿͻ��˵ĵ��ã��������Ȼ��Ҫ���Լ���protobuf��ʽ,��ͷ�����Զ���һ����ʶ
        if(users[client_socket].process()==false){
            goto end;
        }
    }

end:

    client->close();

}
