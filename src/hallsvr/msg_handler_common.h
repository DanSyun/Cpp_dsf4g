#pragma once

#include "msg_handler_basic.h"

class HandlerLogin: public MsgHandlerBasic
{
    ENHandlerResult OnMessage(Session* psession);
    ENHandlerResult OnProcessData(Session* psession);
};

