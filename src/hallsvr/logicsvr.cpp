#include "logicsvr.h"
#include "msg.h"
#include "msg_handler_mng.h"
#include "session_mng.h"
#include "timer_mng.h"
#include "player_mng.h"

#include "cipher.hpp"
#include "config_basic.hpp"
#include "proto_log.hpp"

bool LogicSvr::InitLogic()
{
    ProtoLog::Instance()->Init();   // 获取pb日志
    MsgBasic::Init(this);
    signal(SIGSEGV, MsgBasic::SigHandlerSEGV);    // 捕获段错误

    // rsa key
    if (RSACipher::Instance()->GetPubKey("../../config/public.key") == false)
        return false;

    // 消息会话管理器
    if (!SessionManager::Instance()->Init(
        GetConf().max_session_num(),
        GetConf().session_shmkey()))
        return false;

    // 定时器管理器
    /*if (!TimerManager::Instance()->Init(
        GetConf().max_timer_num(),
        GetConf().timer_shmkey()))
        return false;*/

    // 玩家管理器
    if (!PlayerManager::Instance()->Init(
        GetConf().max_player_num(),
        GetConf().player_shmkey()))
        return false;

    if (!LoadConfig())
        return false;

    // 消息处理
    MsgHandlerMng::Instance()->Init();

    return true;
}

bool LogicSvr::LoadConfig()
{
    //if (!TestConfig::Instance()->Init("../../config/test.cfg"))
        //return false;

    return true;
}

void LogicSvr::Reload()
{
    LoadConfig();
}

void LogicSvr::Exit()
{
    SessionManager::Instance()->Release();
    //TimerManager::Instance()->Release();
    PlayerManager::Instance()->Release();
}

void LogicSvr::OnClientMessage(stTcpHead& head, stPBMsg& msg)
{
    PBCSMsg cs_msg;
    if (!MsgBasic::ParseMsgFromClient(msg, cs_msg))
        return;

    // 登录态检查
    if (cs_msg.has_cs_request_login() == false &&
        PlayerManager::Instance()->GetPlayer(cs_msg.uid()) == NULL)
        return;

    Session* psession = SessionManager::Instance()->CreateSession(cs_msg.msg_union_case());
    if (!psession)
        return;

    psession->SetTcpHead(head);
    psession->SetCSMsg(cs_msg);
    psession->set_uid(cs_msg.uid());
    psession->set_process_state(EN_SESSION_ON_MSG);

    MsgHandlerMng::Instance()->Process(psession);
}

void LogicSvr::OnClientOffline(uint64 logic_index)
{
    std::cout << "uid: " << logic_index << " is offline" << std::endl;
    Msg::PlayerLogout(logic_index, false);
}

void LogicSvr::OnServerMessage(stCenterHead& head, stPBMsg& msg)
{
    PBSSMsg ss_msg;
    if (!MsgBasic::ParseMsgFromServer(msg, ss_msg))
        return;

    Session* psession = NULL;
    // 创建session
    if (ss_msg.type() == SS_Request)
    {
        psession = SessionManager::Instance()->CreateSession(ss_msg.msg_union_case());
        if (!psession) return;

        psession->SetCenterHead(head);
        psession->SetSSMsg(ss_msg);
        psession->set_process_state(EN_SESSION_ON_MSG);
    }
    // 恢复session
    else
    {
        psession = SessionManager::Instance()->GetSession(head.session_id);
        if (!psession) return;

        psession->SetSSMsg(ss_msg);
    }

    MsgHandlerMng::Instance()->Process(psession);
}

void LogicSvr::OnInteralMessage(stPBMsg& msg)
{
    PBInterMsg inter_msg;
    if (!MsgBasic::ParseMsgFromInternal(msg, inter_msg))
        return;

    Session* psession = SessionManager::Instance()->CreateSession(inter_msg.msg_union_case());
    if (!psession)
        return;

    psession->SetInterMsg(inter_msg);
    psession->set_process_state(EN_SESSION_ON_MSG);

    MsgHandlerMng::Instance()->Process(psession);
}

void LogicSvr::OnRoutineCheck()
{
    // timer check，精确到100毫秒
    /*static uint64 timer_check = 0;
    if (timer_check + 100 <= GetTimeMsec())
    {
        timer_check = GetTimeMsec();
        TimerManager::Instance()->Tick();
    }*/
    static uint32 session_check = 0;
    if (session_check + EN_SESSION_CHECK_SEC <= GetTime())
    {
        session_check = GetTime();
        SessionManager::Instance()->CheckTimeOut();
    }
}

int main(int argc, char* argv[])
{
    LogicSvr* psvr = new LogicSvr;
    if (psvr->Initialize(argc, argv, EN_SVR_HALL) == 0)
        psvr->Run();

    return 0;
}

