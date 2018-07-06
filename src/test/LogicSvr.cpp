#include <iostream>
#include <string>
#include <stdlib.h>

#include "LogicSvr.h"
#include "proto_log.hpp"
#include "proto_config.hpp"
#include "proto_config_svr.hpp"
#include "player_mng.h"

bool LogicSvr::Init()
{
    if (!PlayerManager::Instance()->Init(20, 11))
        return false;

    srand(time(NULL));
    uint32 size = rand()% 20;
    std::cout << "\ninit size " << size << "\n";
    for (uint32 i = 0; i < size; ++i)
    {
        Player* pplayer = PlayerManager::Instance()->CreatePlayer(10000 + rand()% 10000);
        if (pplayer)
            std::cout << "create " << pplayer->uid() << std::endl;
    }

    return true;
}
void LogicSvr::Reload(){}
void LogicSvr::Exit()
{
    PlayerManager::Instance()->Release();
}
void LogicSvr::OnClientMessage(){}
void LogicSvr::OnClientOffline(){}
void LogicSvr::OnServerMessage(){}
void LogicSvr::OnInteralMessage(){}

int main(int argc, char* argv[])
{
    /*dan::ProtoLog::Instance()->Init();
    dan::TcpConfig::Instance()->Init("../../config/tcp.cfg");
    dan::TestConfig::Instance()->Init("../../config/test.cfg");


    uint32 size = 10;
    PlayerManager::Instance()->Init(size, 0);
    for (uint32 i = 0; i < size + 10; ++i)
    {
        Player* pplayer = PlayerManager::Instance()->CreatePlayer(10000 + i);
    }
    for (auto iter = PlayerManager::Instance()->Begin(); iter != PlayerManager::Instance()->End(); ++iter)
    {
        Player player = *iter;
        std::cout << player.uid() << "\t" << std::endl;
        player.set_uid(123);
        std::cout << iter->uid() << "\t" << std::endl;
    }*/


    LogicSvr* psvr = new LogicSvr;
    if (psvr->Initialize(argc, argv, svr_hall))
        psvr->Run();

    return 0;
}

