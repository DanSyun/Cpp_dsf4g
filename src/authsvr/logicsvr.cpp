#include "logicsvr.h"
#include "msg_basic.h"
#include "db_mng.h"

#include "cipher.hpp"
#include "config_basic.hpp"
#include "proto_log.hpp"

bool LogicSvr::InitLogic()
{
    ProtoLog::Instance()->Init();   // 获取pb日志
    MsgBasic::Init(this);
    signal(SIGSEGV, MsgBasic::SigHandlerSEGV);    // 捕获段错误

    // rsa key
    if (RSACipher::Instance()->GetPriKey("../../config/private.key") == false)
        return false;

    if (!LoadConfig())
        return false;

    // 连接db
    if (DBManager::Instance()->Init() == false)
        return false;

    return true;
}

bool LogicSvr::LoadConfig()
{
    return true;
}

void LogicSvr::Reload()
{
    LoadConfig();
}

void LogicSvr::Exit()
{
}

void LogicSvr::OnClientMessage(stTcpHead& head, stPBMsg& msg)
{
    PBCSMsg cs_msg;
    if (!MsgBasic::ParseMsgFromClient(msg, cs_msg))
        return;

    DBManager::Instance()->OnMessage(head, cs_msg);
}

void LogicSvr::OnClientOffline(uint64 logic_index)
{
}

void LogicSvr::OnServerMessage(stCenterHead& head, stPBMsg& msg)
{
}

void LogicSvr::OnInteralMessage(stPBMsg& msg)
{
}

void LogicSvr::OnRoutineCheck()
{
}

int main(int argc, char* argv[])
{
    LogicSvr* psvr = new LogicSvr;
    if (psvr->Initialize(argc, argv, EN_SVR_AUTH) == 0)
        psvr->Run();

    return 0;
}

