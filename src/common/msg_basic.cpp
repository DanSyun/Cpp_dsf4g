#include "msg_basic.h"
#include "proto_log.hpp"

Server* MsgBasic::_pserver = NULL;

//////////////////////////////////////////////////////////////////////////
// 解包方法
bool MsgBasic::ParseMsgFromClient(const stPBMsg& msg, PBCSMsg& cs_msg)
{
    if (!cs_msg.ParseFromArray(msg.data, msg.data_len))
        return false;

    TraceProto(cs_msg, cs_msg.msg_union_case(), "GetMsgFromClient");
    return true;
}

bool MsgBasic::ParseMsgFromServer(const stPBMsg& msg, PBSSMsg& ss_msg)
{
    if (!ss_msg.ParseFromArray(msg.data, msg.data_len))
    {
        std::cout << ProtoLog::Instance()->err << std::endl;
        return false;
    }

    TraceProto(ss_msg, ss_msg.msg_union_case(), "GetMsgFromServer");
    return true;
}

bool MsgBasic::ParseMsgFromInternal(const stPBMsg& msg, PBInterMsg& inter_msg)
{
    if (!inter_msg.ParseFromArray(msg.data, msg.data_len))
        return false;

    TraceProto(inter_msg, inter_msg.msg_union_case(), "GetMsgFromInternal");
    return true;
}

//////////////////////////////////////////////////////////////////////////
// 发包方法
bool MsgBasic::SendMsgToClient(Session* psession, bool close)
{
    return SendMsgToClient(psession->GetTcpHead(), psession->GetCSResponse(), close);
}

bool MsgBasic::SendMsgToClient(stTcpHead& head, PBCSMsg& cs_msg, bool close)
{
    uint32 buf_len = cs_msg.ByteSize();
    if (buf_len > MAX_PB_LEN) return false;
    char buf[MAX_PB_LEN];

    head.msg_type = TCP_NORMAL;
    head.need_close = close;

    cs_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    TraceProto(cs_msg, cs_msg.msg_union_case(), "SendMsgToClient");
    return _pserver->SendMessageToClient(head, buf, buf_len);
}

bool MsgBasic::SendMsgToResponse(Session* psession)
{
    return SendMsgToResponse(psession->GetCenterHead(), psession->GetSSResponse());
}

bool MsgBasic::SendMsgToResponse(stCenterHead& src_head, PBSSMsg& ss_msg)
{
    ss_msg.set_type(SS_Response);
    uint32 buf_len = ss_msg.ByteSize();
    if (buf_len > MAX_PB_LEN) return false;

    stCenterHead head;
    head.src_type       = _pserver->GetSvrType();
    head.src_id         = _pserver->GetSvrID();
    head.des_type       = src_head.src_type;
    head.des_id         = src_head.src_id;
    head.session_id     = src_head.session_id;
    head.route_type     = route_p2p;

    char buf[MAX_PB_LEN];
    ss_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    return _pserver->SendMessageToCenter(head, buf, buf_len, src_head.center_index);
}

bool MsgBasic::SendMsgToInternal(const PBInterMsg& inter_msg)
{
    uint32 buf_len = inter_msg.ByteSize();
    if (buf_len > MAX_PB_LEN) return false;
    char buf[MAX_PB_LEN];

    inter_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    return _pserver->SendMessageToInternal(buf, buf_len);
}

bool MsgBasic::SendMsgToHallById(Session* psession, PBSSMsg& ss_msg, uint32 svr_id)
{
    ss_msg.set_type(SS_Request);
    uint32 buf_len = ss_msg.ByteSize();
    if (buf_len > MAX_PB_LEN) return false;

    stCenterHead head;
    head.src_type       = _pserver->GetSvrType();
    head.src_id         = _pserver->GetSvrID();
    head.des_type       = EN_SVR_HALL;
    head.des_id         = svr_id;
    head.session_id     = psession->session_id();
    head.route_type     = route_p2p;

    char buf[MAX_PB_LEN];
    ss_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    psession->set_update_time(GetTime());
    psession->set_process_state(EN_SESSION_WAIT_SVR_RSP);
    return _pserver->SendMessageToCenter(head, buf, buf_len);
}

bool MsgBasic::SendMsgToDBProxy(Session* psession, PBSSMsg& ss_msg)
{
    ss_msg.set_type(SS_Request);
    uint32 buf_len = ss_msg.ByteSize();
    if (buf_len > MAX_PB_LEN) return false;

    stCenterHead head;
    head.src_type       = _pserver->GetSvrType();
    head.src_id         = _pserver->GetSvrID();
    head.des_type       = EN_SVR_DBPROXY;
    head.session_id     = psession->session_id();
    head.route_type     = route_hash;
    head.hash_key       = ss_msg.uid();

    char buf[MAX_PB_LEN];
    ss_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    psession->set_update_time(GetTime());
    return _pserver->SendMessageToCenter(head, buf, buf_len);
}

//////////////////////////////////////////////////////////////////////////
// 定时器接口
int MsgBasic::SetTimer(uint32 sec, const PBInterMsg& inter)
{
    return SetTimerMsec(sec* 1000, inter);
}

int MsgBasic::SetTimerMsec(uint32 msec, const PBInterMsg& inter)
{
    return TimerManager::Instance()->CreateTimer(msec, inter);
}

void MsgBasic::ClearTimer(int timer_id)
{
    if (timer_id < 0) return;
    TimerManager::Instance()->ReleaseTimer(timer_id);
}

//////////////////////////////////////////////////////////////////////////
// db 处理接口
template<class PB_TYPE>
static bool AddSaveData(SSRequestSave& request, uint32 key, PB_TYPE& pb)
{
    PB_TYPE tmp;
    tmp.CopyFrom(pb);
    tmp.clear_head();
    uint32 len = tmp.ByteSize();
    if (len > EN_MAX_PB_DATA_BUF)
        return false;

    // 序列化
    char buf[EN_MAX_PB_DATA_BUF];
    tmp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

    // 本地版本号 + 1
    pb.mutable_head()->set_ver(pb.head().ver() + 1);

    PBDBData& db_data = *request.add_datas();
    db_data.set_key(key);
    db_data.set_ver(pb.head().ver());
    db_data.set_blob(buf, len);

    return true;
}

template<class PB_TYPE>
static bool ConstructDBMsg(PB_TYPE& pb, const PBDBData& data)
{
    // 必须先解析blob
    if (data.has_blob())
    {
        if (!pb.ParseFromArray(data.blob().c_str(), data.blob().size()))
            return false;
    }
    // 填充version
    if (data.has_ver())
    {
        pb.mutable_head()->set_ver(data.ver());
    }
    return true;
}

bool MsgBasic::SendGetToDBHandle(Session * psession)
{
    bool get = false;
    for (uint32 i = 0; i < psession->db_procs_size(); ++i)
    {
        const PBDBProcInfo& proc = psession->db_procs(i);
        // 只请求没有本地数据的uid
        if (proc.has_player())
            continue;

        PBSSMsg msg;
        msg.set_uid(proc.uid());
        msg.mutable_ss_request_get()->set_uid(proc.uid());
        for (uint32 i = 0; i < proc.keys_size(); ++i)
        {
            PBDBData& data = *msg.mutable_ss_request_get()->add_datas();
            data.set_key(proc.keys(i)); 
        }

        get = true;
        SendMsgToDBProxy(psession, msg);
    }
    if (get)
    {
        psession->set_process_state(EN_SESSION_WAIT_GET_RSP);
    }

    return get;
}

bool MsgBasic::SendSaveToDBHandle(Session * psession)
{
    if (psession->cur_proc_index() >= psession->db_procs_size())
        return false;

    bool save = false;
    PBDBMsg& player = psession->GetDBMsg();
    PBDBProcInfo& proc = psession->GetDBProcInfo();

    PBSSMsg msg;
    SSRequestSave& request = *msg.mutable_ss_request_save();
    msg.set_uid(player.uid());
    request.set_uid(player.uid());
    // save 的必须是get 的key，有改动的(mutable) 才save
    for (uint32 i = 0; i < proc.keys_size(); ++i)
    {
        switch (proc.keys(i))
        {
            case PBDBKey::kBaseInfo:
                if (player.update_base_info() && AddSaveData(request, proc.keys(i), *player.mutable_base_info()))
                {
                    save = true;
                    player.clear_update_base_info();
                }
                break;
            case PBDBKey::kMoneyInfo:
                if (player.update_money_info() && AddSaveData(request, proc.keys(i), *player.mutable_money_info()))
                {
                    save = true;
                    player.clear_update_money_info();
                }
                break;
            default: return false;
        }
    }
    if (save)
    {
        SendMsgToDBProxy(psession, msg);
        psession->set_process_state(EN_SESSION_WAIT_SAVE_RSP);
    }

    return save;
}

bool MsgBasic::ParseDBData(PBDBMsg& player, const google::protobuf::RepeatedPtrField<PBDBData>& datas)
{
    for (uint32 i = 0; i < datas.size(); ++i)
    {
        const PBDBData& data = datas.Get(i);
        switch (data.key())
        {
            case PBDBKey::kBaseInfo:
                if (!ConstructDBMsg(*player.mutable_base_info(), data))
                    return false;
                player.clear_update_base_info();
                break;
            case PBDBKey::kMoneyInfo:
                if (!ConstructDBMsg(*player.mutable_money_info(), data))
                    return false;
                player.clear_update_money_info();
                break;
            default: return false;
        }
    }
    return true;
}

void MsgBasic::InitDBData(Session* psession)
{
    PBDBMsg& player = psession->GetDBMsg();
    player.mutable_base_info()->set_is_init(true);
    player.mutable_money_info()->set_money(1000000);
}

