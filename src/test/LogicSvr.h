#pragma once

#include "server.h"

class LogicSvr: public Server
{
public:
    virtual bool Init();
    virtual void Reload();              // �ź���������
    virtual void Exit();                // �ź��˳�
    virtual void OnClientMessage();     // �ͻ�����Ϣ
    virtual void OnClientOffline();     // �ͻ��˶���
    virtual void OnServerMessage();     // center��Ϣ
    virtual void OnInteralMessage();    // �ڲ��첽��Ϣ
};

