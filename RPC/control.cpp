#include"control.h"

Control::Control() {
    m_failed=false;
    m_errText="";
}

void Control::Reset() {
    m_failed=false;
    m_errText="";
}

std::string Control::ErrorText() const {
    return m_errText;
}

void Control::SetFailed(const std::string &reason) {
    m_failed=true;
    m_errText=reason;
}

bool Control::Failed() const {
    return m_failed;
}

// 目前未实现具体的功能
void Control::StartCancel() {}
bool Control::IsCanceled() const { return false; }
void Control::NotifyOnCancel(google::protobuf::Closure* callback) {}