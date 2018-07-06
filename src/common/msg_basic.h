#pragma once

#include "server.h"
#include "timer_mng.h"
#include "player_mng.h"
#include "session_mng.h"

class MsgBasic
{
protected:
    static Server* _pserver;
public:
    static void Init(Server* pserver)       { _pserver = pserver; }
    static void SigHandlerSEGV(int sig)     { _pserver->Exit(); }
    static uint32 GetSvrType()              { return _pserver->GetSvrType(); }
    static uint32 GetSvrID()                { return _pserver->GetSvrID(); }
    static const PBLogicConfig& GetConf()   { return _pserver->GetConf(); }

    // proto 解析
    static bool ParseMsgFromClient(const stPBMsg& msg, PBCSMsg& cs_msg);
    static bool ParseMsgFromServer(const stPBMsg& msg, PBSSMsg& ss_msg);
    static bool ParseMsgFromInternal(const stPBMsg& msg, PBInterMsg& inter_msg);
    // 发包函数
    static bool SendMsgToClient(Session* psession, bool close = false);
    static bool SendMsgToClient(stTcpHead& head, PBCSMsg& cs_msg, bool close = false);

    static bool SendMsgToResponse(Session* psession);
    static bool SendMsgToResponse(stCenterHead& src_head, PBSSMsg& ss_msg);

    static bool SendMsgToInternal(const PBInterMsg& inter_msg);

    static bool SendMsgToHallById(Session* psession, PBSSMsg& ss_msg, uint32 svr_id);
    static bool SendMsgToDBProxy(Session* psession, PBSSMsg& ss_msg);
    // 定时器接口
    static int SetTimer(uint32 sec, const PBInterMsg& inter);
    static int SetTimerMsec(uint32 msec, const PBInterMsg& inter);
    static void ClearTimer(int timer_id);
    // db process 相关接口
    static bool SendGetToDBHandle(Session* psession);
    static bool SendSaveToDBHandle(Session* psession);
    static bool ParseDBData(PBDBMsg& player, const google::protobuf::RepeatedPtrField<PBDBData>& datas);
    static void InitDBData(Session* psession);
};

