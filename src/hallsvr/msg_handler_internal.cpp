#include "msg_handler_internal.h"
#include "msg.h"

ENHandlerResult HandlerLogout::OnMessage(Session* psession)
{
    uint64 uid = psession->GetCSMsg().uid();
    psession->AddDBGetKey(uid, PBDBKey::kBaseInfo);
    psession->AddDBGetKey(uid, PBDBKey::kMoneyInfo);

    return EN_HANDLER_GET;
}
ENHandlerResult HandlerLogout::OnProcessData(Session* psession)
{
    PBDBMsg& player = psession->GetDBMsg();
    player.mutable_base_info()->clear_is_online();
    player.mutable_base_info()->clear_hallsvr_id();

    return EN_HANDLER_SAVE;
}
ENHandlerResult HandlerLogout::OnSaveSuccess(Session * psession)
{
    return EN_HANDLER_DONE;
}

