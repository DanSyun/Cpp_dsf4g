#include "pb_cs_msg.pb.h"
#include "pb_ss_msg.pb.h"
#include "pb_inter_msg.pb.h"
#include "msg_handler_mng.h"

void MsgHandlerMng::Init()
{
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

