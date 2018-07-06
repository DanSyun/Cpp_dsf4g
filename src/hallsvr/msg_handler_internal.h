#pragma once

#include "msg_handler_basic.h"

class HandlerLogout: public MsgHandlerBasic
{
    ENHandlerResult OnMessage(Session* psession);
    ENHandlerResult OnProcessData(Session* psession);
    ENHandlerResult OnSaveSuccess(Session * psession);
};

