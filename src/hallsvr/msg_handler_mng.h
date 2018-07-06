#pragma once

#include <unordered_map>

#include "singleton.hpp"
#include "session_mng.h"
#include "msg_handler_basic.h"


class MsgHandlerMng: public Singleton<MsgHandlerMng>
{
public:
    void Init();
    void Process(Session* psession);
private:
    std::unordered_map<uint32, MsgHandlerBasic*> _handlers;
};
