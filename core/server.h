#pragma once

#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>

#include "base.h"
#include "config.h"
#include "shm_pipe.h"
#include "share_mem.hpp"
#include "center_mng.h"

#define MAX_CENTER_NUM  (16)

class Server
{
private:
    void ParseArgs(int argc, char* argv[], char* conf);
    bool LoadConfig(const char* conf);
    bool InitMsgPipe();
    bool InitCsPipe();
    bool InitCenter();

    int CheckRunFlag();
    void CheckCenterConn();
    void GetClientMessage();
    void GetCenterMessage();
    bool PostMessage(stTcpHead& head, char* buf, uint32 buf_len);
    bool PostMessage(stCenterHead& head, char* buf, uint32 buf_len);
    bool PostMessage(char* buf, uint32 buf_len);
    void DispatchMessage();

public:
    virtual bool InitLogic() = 0;           // ����ҵ���ʼ��
    virtual void Reload() = 0;              // �ź���������
    virtual void Exit() = 0;                // �ź��˳�
    virtual void OnClientMessage(stTcpHead& head, stPBMsg& msg) = 0;    // �ͻ�����Ϣ
    virtual void OnClientOffline(uint64 logic_index) = 0;               // �ͻ��˶���
    virtual void OnServerMessage(stCenterHead& head, stPBMsg& msg) = 0; // center��Ϣ
    virtual void OnInteralMessage(stPBMsg& msg) = 0;                    // �ڲ��첽��Ϣ
    virtual void OnRoutineCheck() = 0;      // timer

public:
    int Initialize(int argc, char* argv[], uint32 type);
    void Run();
    const SvrConfig& GetConf()  { return _conf; }
    uint32 GetSvrType()         { return _info.svr_type; }
    uint32 GetSvrID()           { return _info.svr_id; }

    // ����ӿ�
    bool SendMessageToInternal(char* buf, uint32 len);
    bool SendMessageToClient(stTcpHead& head, char* buf, uint32 len);
    bool SendMessageToCenter(stCenterHead& head, char* buf, uint32 len, int center_index = -1);
    bool NotifyTcpLogicIndex(stTcpHead& head, uint64 logic_index);
private:
    stSvrInfo   _info;
    SvrConfig   _conf;

    ShmPipe* pcs_pipe;
    ShmPipe* psc_pipe;
    ShmPipe* pmsg_pipe;

    char _tcp_buf[TCP_BUF_LEN];
    uint32 _tcp_len;
    stMsg _msg;
    uint32 _msg_len;

    uint32 _center_num;
    CenterMng _center_mng[MAX_CENTER_NUM];
    char _center_buf[CENTER_BUF_LEN];
    uint32 _center_len;
};

