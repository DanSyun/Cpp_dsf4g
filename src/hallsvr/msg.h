#pragma once

#include "msg_basic.h"

class Msg: public MsgBasic
{
public:
    static bool PlayerLogin(Session* psession);
    static void PlayerLogout(uint64 uid, bool notify_cli = true);
    static void PlayerLogout(Player* pplayer, bool notify_cli = true);
};

