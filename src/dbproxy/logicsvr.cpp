#include "logicsvr.h"
#include "msg_basic.h"

#include "session_mng.h"
#include "timer_mng.h"
#include "player_mng.h"

#include "config_basic.hpp"
#include "proto_log.hpp"
#include "service.h"
#include "pb_ss_msg.pb.h"
bool DBServer::InitLogic()
{
    ProtoLog::Instance()->Init();   // 获取pb日志
    MsgBasic::Init(this);
    signal(SIGSEGV, MsgBasic::SigHandlerSEGV);// 捕获段错误

    return CService::Instance().Init();
}

bool DBServer::LoadConfig()
{
	return true;
}

void DBServer::Reload()
{
    CService::Instance().Uninit();
}

void DBServer::Exit()
{
   
}

void DBServer::OnClientMessage(stTcpHead& head, stPBMsg& msg)
{
   
}

void DBServer::OnClientOffline(uint64 logic_index)
{
   
}

void DBServer::OnServerMessage(stCenterHead& head, stPBMsg& msg)
{
    PBSSMsg ss_msg;
    if (!MsgBasic::ParseMsgFromServer(msg, ss_msg))
        return;

	CService::Instance().Message(head, ss_msg);
}

void DBServer::OnInteralMessage(stPBMsg& msg)
{
   
}

void DBServer::OnRoutineCheck()
{
}

int main(int argc, char* argv[])
{
    DBServer* psvr = new DBServer;
    if (psvr->Initialize(argc, argv, EN_SVR_DBPROXY) == 0)
        psvr->Run();

    return 0;
}

