#include "msg_handler_common.h"
#include "player_mng.h"
#include "msg.h"

ENHandlerResult HandlerLogin::OnMessage(Session* psession)
{
    // 先验证请求
    if (Msg::PlayerLogin(psession) == false)
    {
        psession->GetCSResponse().mutable_cs_response_login()->set_result(EN_MESSAGE_LOGIN_FAILED);
        MsgBasic::SendMsgToClient(psession, true);
        return EN_HANDLER_DONE;
    }

    uint64 uid = psession->GetCSMsg().uid();
    psession->AddDBGetKey(uid, PBDBKey::kBaseInfo);
    psession->AddDBGetKey(uid, PBDBKey::kMoneyInfo);

    return EN_HANDLER_GET;
}
ENHandlerResult HandlerLogin::OnProcessData(Session* psession)
{
    PBDBMsg& player = psession->GetDBMsg();
    // 注册数据
    if (player.base_info().is_init() == false)
    {
        MsgBasic::InitDBData(psession);
    }
    player.mutable_base_info()->set_is_online(true);
    player.mutable_base_info()->set_hallsvr_id(MsgBasic::GetSvrID());
    //player.mutable_money_info()->set_money(rand());

    psession->GetCSResponse().mutable_cs_response_login()->set_result(EN_MESSAGE_OK);
    psession->GetCSResponse().mutable_cs_response_login()->set_money(psession->GetDBMsg().money_info().money());

    return EN_HANDLER_SAVE;
}

