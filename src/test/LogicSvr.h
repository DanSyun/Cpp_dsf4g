#pragma once

#include "server.h"

class LogicSvr: public Server
{
public:
    virtual bool Init();
    virtual void Reload();              // 信号重载配置
    virtual void Exit();                // 信号退出
    virtual void OnClientMessage();     // 客户端消息
    virtual void OnClientOffline();     // 客户端断线
    virtual void OnServerMessage();     // center消息
    virtual void OnInteralMessage();    // 内部异步消息
};

