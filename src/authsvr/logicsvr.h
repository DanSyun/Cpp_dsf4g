#pragma once

#include "server.h"

class LogicSvr: public Server
{
public:
    virtual bool InitLogic();
    virtual void Reload();              // 信号重载配置
    virtual void Exit();                // 信号退出
    virtual void OnClientMessage(stTcpHead& head, stPBMsg& msg);    // 客户端消息
    virtual void OnClientOffline(uint64 logic_index);               // 客户端断线
    virtual void OnServerMessage(stCenterHead& head, stPBMsg& msg); // center消息
    virtual void OnInteralMessage(stPBMsg& msg);                    // 内部异步消息
    virtual void OnRoutineCheck();      // timer
private:
    bool LoadConfig();
};

