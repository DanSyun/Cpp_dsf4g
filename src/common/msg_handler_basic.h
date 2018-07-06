#pragma once

#include <string>
#include "session_mng.h"

class MsgHandlerBasic
{
public:
    void Process(Session* psession);

    // 子类覆盖接口
    virtual ENHandlerResult OnMessage(Session* psession);       // 收到消息
    virtual ENHandlerResult OnSvrResponse(Session* psession);   // svr异步返回
    virtual ENHandlerResult OnProcessData(Session* psession);   // 处理db数据
    virtual ENHandlerResult OnSaveSuccess(Session* psession);   // db更新成功
    virtual ENHandlerResult OnGetFailed(Session* psession);
    virtual ENHandlerResult OnSaveFailed(Session* psession);
    // 内部流程实现
    ENHandlerResult GetAllData(Session* psession);
    ENHandlerResult SaveOneData(Session* psession);
    ENHandlerResult GetResponse(Session* psession);
    ENHandlerResult SaveResponse(Session* psession);
    ENHandlerResult ProcessNext(Session* psession);
    void ProcessDone(Session* psession);
    void ProcessFailed(Session* psession);

private:
    std::string err_msg;
};

