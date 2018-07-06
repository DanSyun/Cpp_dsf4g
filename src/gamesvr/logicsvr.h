#pragma once

#include "server.h"

class LogicSvr: public Server
{
public:
    virtual bool InitLogic();
    virtual void Reload();              // �ź���������
    virtual void Exit();                // �ź��˳�
    virtual void OnClientMessage(stTcpHead& head, stPBMsg& msg);    // �ͻ�����Ϣ
    virtual void OnClientOffline(uint64 logic_index);               // �ͻ��˶���
    virtual void OnServerMessage(stCenterHead& head, stPBMsg& msg); // center��Ϣ
    virtual void OnInteralMessage(stPBMsg& msg);                    // �ڲ��첽��Ϣ
    virtual void OnRoutineCheck();      // timer
private:
    bool LoadConfig();
};

