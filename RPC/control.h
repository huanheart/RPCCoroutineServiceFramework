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
    bool m_failed;   //rpc执行过程中的状态
    std::string m_errText;    //rpc执行过程中的错误信息
};

