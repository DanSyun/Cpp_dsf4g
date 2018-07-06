#pragma once

#include <string>
#include "session_mng.h"

class MsgHandlerBasic
{
public:
    void Process(Session* psession);

    // ���า�ǽӿ�
    virtual ENHandlerResult OnMessage(Session* psession);       // �յ���Ϣ
    virtual ENHandlerResult OnSvrResponse(Session* psession);   // svr�첽����
    virtual ENHandlerResult OnProcessData(Session* psession);   // ����db����
    virtual ENHandlerResult OnSaveSuccess(Session* psession);   // db���³ɹ�
    virtual ENHandlerResult OnGetFailed(Session* psession);
    virtual ENHandlerResult OnSaveFailed(Session* psession);
    // �ڲ�����ʵ��
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

