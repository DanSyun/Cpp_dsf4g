#include <iostream>
#include "msg_handler_basic.h"
#include "msg_basic.h"

void MsgHandlerBasic::Process(Session* psession)
{
    ENHandlerResult result = EN_HANDLER_DONE;
    switch (psession->process_state())
    {
        case EN_SESSION_ON_MSG:             result = OnMessage(psession);           break;
        case EN_SESSION_WAIT_SVR_RSP:       result = OnSvrResponse(psession);       break;

        case EN_SESSION_WAIT_GET_RSP:       result = GetResponse(psession);         break;
        case EN_SESSION_WAIT_SAVE_RSP:      result = SaveResponse(psession);        break;
        default: break;
    }

    while (true)
    {
        switch (result)
        {
            case EN_HANDLER_GET:            result = GetAllData(psession);      break;
            case EN_HANDLER_SAVE:           result = SaveOneData(psession);     break;
            case EN_HANDLER_NEXT:           result = ProcessNext(psession);   break;
            case EN_HANDLER_DONE:           ProcessDone(psession);            return;
            case EN_HANDLER_FAILED:         ProcessFailed(psession);          return;
            case EN_HANDLER_HUP:            /* do nothing */                    return;
            default: break;
        }
    }
}

ENHandlerResult MsgHandlerBasic::OnMessage(Session* psession)
{
    // 默认拉取请求玩家所有数据
    psession->AddDBGetKey(psession->uid(), PBDBKey::kBaseInfo);
    psession->AddDBGetKey(psession->uid(), PBDBKey::kMoneyInfo);
    return EN_HANDLER_GET;
}
ENHandlerResult MsgHandlerBasic::OnSvrResponse(Session* psession)
{
    // sub class should overwrite this
    return EN_HANDLER_DONE;
}
ENHandlerResult MsgHandlerBasic::OnProcessData(Session * psession)
{
    // sub class should overwrite this
    return EN_HANDLER_DONE;
}
ENHandlerResult MsgHandlerBasic::OnSaveSuccess(Session * psession)
{
    // 默认回复玩家消息
    MsgBasic::SendMsgToClient(psession);
    return EN_HANDLER_DONE;
}
ENHandlerResult MsgHandlerBasic::OnGetFailed(Session* psession)
{
    // sub class should overwrite this
    return EN_HANDLER_FAILED;
}
ENHandlerResult MsgHandlerBasic::OnSaveFailed(Session* psession)
{
    // sub class should overwrite this
    return EN_HANDLER_FAILED;
}

ENHandlerResult MsgHandlerBasic::ProcessNext(Session* psession)
{
    // 如果还有可处理的玩家数据则继续否则结束会话
    psession->set_cur_proc_index(psession->cur_proc_index() + 1);
    if (psession->cur_proc_index() < psession->db_procs_size())
    {
        return OnProcessData(psession);
    }
    else
    {
        return EN_HANDLER_DONE;
    }
}

void MsgHandlerBasic::ProcessDone(Session* psession)
{
    SessionManager::Instance()->ReleaseSession(psession->session_id());
    // post 日志消息
}

void MsgHandlerBasic::ProcessFailed(Session* psession)
{
    SessionManager::Instance()->ReleaseSession(psession->session_id());
}


// db返回
ENHandlerResult MsgHandlerBasic::GetResponse(Session* psession)
{
    const SSResponseGet& response = psession->GetSSMsg().ss_response_get();
    // 拉取成功
    if (response.result() == E_HANDLE_OK)
    {
        // 解包
        PBDBMsg player;
        player.set_uid(response.uid());
        if (!MsgBasic::ParseDBData(player, response.datas()))
            return OnGetFailed(psession);

        int proc_index = psession->GetDBProcIndex(response.uid());
        if (proc_index == -1)
            return OnGetFailed(psession);

        psession->GetDBMsg(proc_index).CopyFrom(player);

        // 如果拉到所有数据开始处理
        if (psession->GetAllDBData())
        {
            return OnProcessData(psession);
        }
        // 否则继续挂起等待回复
        else
        {
            return EN_HANDLER_HUP;
        }
    }
    // db 错误
    else
    {
        return OnGetFailed(psession);
    }
}

ENHandlerResult MsgHandlerBasic::SaveResponse(Session* psession)
{
    const SSResponseSave& response = psession->GetSSMsg().ss_response_save();
    // 保存成功
    if (response.result() == E_HANDLE_OK)
    {
        return OnSaveSuccess(psession);
    }
    // 版本号不一致，重做
    else if (response.result() == E_HANDLE_VER_DIFFER)
    {
        // 重做次数限制
        psession->set_redo_count(psession->redo_count() + 1);
        if (psession->redo_count() >= EN_MAX_DB_PROC_REDO)
        {
            return OnSaveFailed(psession);
        }
        // 用新数据重做逻辑
        PBDBMsg player;
        player.set_uid(response.uid());
        if (!MsgBasic::ParseDBData(player, response.datas()))
            return OnSaveFailed(psession);

        int proc_index = psession->GetDBProcIndex(response.uid());
        if (proc_index == -1)
            return OnSaveFailed(psession);

        psession->GetDBMsg(proc_index).CopyFrom(player);

        return OnProcessData(psession);
    }
    // db 错误
    else
    {
        return OnSaveFailed(psession);
    }
}

// 拉取所有数据
ENHandlerResult MsgHandlerBasic::GetAllData(Session* psession)
{
    if (MsgBasic::SendGetToDBHandle(psession))
    {
        return EN_HANDLER_HUP;
    }
    else //没有设置要拉取的数据
    {
        return EN_HANDLER_FAILED;
    }
}
// 保存单个玩家数据
ENHandlerResult MsgHandlerBasic::SaveOneData(Session* psession)
{
    if (MsgBasic::SendSaveToDBHandle(psession))
    {
        return EN_HANDLER_HUP;
    }
    else //没有被改动的数据
    {
        return EN_HANDLER_FAILED;
    }
}

