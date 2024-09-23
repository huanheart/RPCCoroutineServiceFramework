#pragma  once

#include <google/protobuf/service.h>
#include<string>

class Control :public google::protobuf::RpcController
{
public:
    Control();
    void Reset() override;
    bool Failed() const override;
    std::string ErrorText() const override;
    void SetFailed(const std::string & reason) override;

    void StartCancel() override;
    bool IsCanceled() const override;
    void NotifyOnCancel(google::protobuf::Closure * callback) override;
private:
    bool m_failed;   //rpcִ�й����е�״̬
    std::string m_errText;    //rpcִ�й����еĴ�����Ϣ
};

