#pragma once

#include <unordered_map>
#include "object_mng.hpp"
#include "singleton.hpp"
#include "pb_object.pb.h"
#include "base.h"

class Session: public PBSession
{
private:
    typedef PBSession base;
public:
    size_t GetByteSize();
    bool Serialize(uint8* buf, size_t size, size_t& offset);
    bool UnSerialize(uint8* buf, size_t size, size_t& offset);

    void AddDBGetKey(uint64 uid, uint32 key);
    int GetDBProcIndex(uint64 uid);
    bool GetAllDBData();

    stTcpHead       & GetTcpHead()          { return _msg_head.tcp_head; }
    stCenterHead    & GetCenterHead()       { return _msg_head.center_head; }
    PBCSMsg         & GetCSMsg()            { return *base::mutable_cs_msg(); }
    PBCSMsg         & GetCSResponse()       { return *base::mutable_cs_response(); }
    PBSSMsg         & GetSSMsg()            { return *base::mutable_ss_msg(); }
    PBSSMsg         & GetSSResponse()       { return *base::mutable_ss_response(); }

    PBDBProcInfo    & GetDBProcInfo()       { return *base::mutable_db_procs(base::cur_proc_index()); }
    PBDBMsg         & GetDBMsg()            { return *base::mutable_db_procs(base::cur_proc_index())->mutable_player(); }
    PBDBMsg         & GetDBMsg(uint32 index){ return *base::mutable_db_procs(index)->mutable_player(); }

    void            SetTcpHead(const stTcpHead& head)       { _msg_head.tcp_head = head; }
    void            SetCenterHead(const stCenterHead& head) { _msg_head.center_head = head; }
    void            SetCSMsg(const PBCSMsg& msg)            { base::mutable_cs_msg()->CopyFrom(msg); }
    void            SetSSMsg(const PBSSMsg& msg)            { base::mutable_ss_msg()->CopyFrom(msg); }
    void            SetInterMsg(const PBInterMsg& msg)      { base::mutable_inter_msg()->CopyFrom(msg); }
private:
    // 非pb对象也要序列化
    uMsgHead    _msg_head;
};

class SessionManager:
    public ObjectMng<Session>,
    public Singleton<SessionManager>
{
public:
    typedef ObjectMng<Session> base;
    typedef std::unordered_map<uint32, uint32> index;
public:
    bool Init(uint32 size, uint32 shmkey = 0);

    Session* CreateSession(uint32 msg_id);
    Session* GetSession(uint32 session_id);
    bool ReleaseSession(uint32 session_id);
    void CheckTimeOut();

private:
    index   _index;
    uint32  _cur_max_id;
};

