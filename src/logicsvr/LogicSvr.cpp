#include <iostream>
#include <string>
#include <stdlib.h>

#include "LogicSvr.h"
#include "proto_log.hpp"
#include "proto_config.hpp"
#include "player_mng.h"

bool LogicSvr::Init()
{
    if (!LoadConfig())
        return false;

    if (!PlayerManager::Instance()->Init(dan::SvrConfig::Instance()->max_player_num(),
                                        dan::SvrConfig::Instance()->player_shmkey()))
        return false;

    srand(time(NULL));
    uint32 size = rand()% 20;
    std::cout << "init size " << size << "\n";
    for (uint32 i = 0; i < size; ++i)
    {
        Player* pplayer = PlayerManager::Instance()->CreatePlayer(10000 + rand()% 10000);
        if (pplayer)
            dan::TraceProto(*pplayer);
    }

    return true;
}

bool LogicSvr::LoadConfig()
{
    if (!dan::TestConfig::Instance()->Init("../../config/test.cfg"))
        return false;
    dan::TraceProto(*dan::TestConfig::Instance());

    return true;
}

void LogicSvr::Reload()
{
    LoadConfig();
}

void LogicSvr::Exit()
{
    PlayerManager::Instance()->Release();
}

void LogicSvr::OnClientMessage(){}
void LogicSvr::OnClientOffline(){}
void LogicSvr::OnServerMessage(){}
void LogicSvr::OnInteralMessage(){}
void LogicSvr::OnRoutineCheck(){}

int main(int argc, char* argv[])
{
    LogicSvr* psvr = new LogicSvr;
    if (psvr->Initialize(argc, argv, svr_hall))
        psvr->Run();

    return 0;
}

