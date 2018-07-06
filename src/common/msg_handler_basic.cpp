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
    // Ĭ����ȡ���������������
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
    // Ĭ�ϻظ������Ϣ
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
    // ������пɴ������������������������Ự
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
    // post ��־��Ϣ
}

void MsgHandlerBasic::ProcessFailed(Session* psession)
{
    SessionManager::Instance()->ReleaseSession(psession->session_id());
}


// db����
ENHandlerResult MsgHandlerBasic::GetResponse(Session* psession)
{
    const SSResponseGet& response = psession->GetSSMsg().ss_response_get();
    // ��ȡ�ɹ�
    if (response.result() == E_HANDLE_OK)
    {
        // ���
        PBDBMsg player;
        player.set_uid(response.uid());
        if (!MsgBasic::ParseDBData(player, response.datas()))
            return OnGetFailed(psession);

        int proc_index = psession->GetDBProcIndex(response.uid());
        if (proc_index == -1)
            return OnGetFailed(psession);

        psession->GetDBMsg(proc_index).CopyFrom(player);

        // ��������������ݿ�ʼ����
        if (psession->GetAllDBData())
        {
            return OnProcessData(psession);
        }
        // �����������ȴ��ظ�
        else
        {
            return EN_HANDLER_HUP;
        }
    }
    // db ����
    else
    {
        return OnGetFailed(psession);
    }
}

ENHandlerResult MsgHandlerBasic::SaveResponse(Session* psession)
{
    const SSResponseSave& response = psession->GetSSMsg().ss_response_save();
    // ����ɹ�
    if (response.result() == E_HANDLE_OK)
    {
        return OnSaveSuccess(psession);
    }
    // �汾�Ų�һ�£�����
    else if (response.result() == E_HANDLE_VER_DIFFER)
    {
        // ������������
        psession->set_redo_count(psession->redo_count() + 1);
        if (psession->redo_count() >= EN_MAX_DB_PROC_REDO)
        {
            return OnSaveFailed(psession);
        }
        // �������������߼�
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
    // db ����
    else
    {
        return OnSaveFailed(psession);
    }
}

// ��ȡ��������
ENHandlerResult MsgHandlerBasic::GetAllData(Session* psession)
{
    if (MsgBasic::SendGetToDBHandle(psession))
    {
        return EN_HANDLER_HUP;
    }
    else //û������Ҫ��ȡ������
    {
        return EN_HANDLER_FAILED;
    }
}
// ���浥���������
ENHandlerResult MsgHandlerBasic::SaveOneData(Session* psession)
{
    if (MsgBasic::SendSaveToDBHandle(psession))
    {
        return EN_HANDLER_HUP;
    }
    else //û�б��Ķ�������
    {
        return EN_HANDLER_FAILED;
    }
}

