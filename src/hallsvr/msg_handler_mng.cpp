#include "msg_handler_mng.h"
#include "msg_handler_common.h"
#include "msg_handler_internal.h"

void MsgHandlerMng::Init()
{
    // �ͻ�����Ϣ
    _handlers[PBCSMsg::kCsRequestLogin]     = new HandlerLogin();

    // �ڲ���Ϣ
    _handlers[PBInterMsg::kInterLogout]     = new HandlerLogout();
}

void MsgHandlerMng::Process(Session * psession)
{
    if (!psession)
        return;

    if (_handlers.find(psession->msg_id()) != _handlers.end())
    {
        _handlers[psession->msg_id()]->Process(psession);
    }
}

