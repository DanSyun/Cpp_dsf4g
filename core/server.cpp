#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

#include "server.h"
#include "daemon.hpp"


static int run_flag = 0;
enum ENRunFlag
{
    EN_Flag_Exit    = 1,
    EN_Flag_Reload  = 2,
    // xx = 4
};
void SignalExit(int sig)
{
    run_flag |= EN_Flag_Exit;
}
void SignalReload(int sig)
{
    run_flag |= EN_Flag_Reload;
}


void Server::ParseArgs(int argc, char* argv[], char* conf)
{
    int opt;
    while ((opt = getopt(argc, argv, "dc:i:")) != -1)
    {
        switch (opt)
        {
            case 'd': Daemonize(); break;
            case 'c': strcpy(conf, optarg); break;
            case 'i': _info.svr_id = atoi(optarg); break;
            default: break;
        }
    }
}

bool Server::LoadConfig(const char* conf)
{
    if (!conf) return false;
    if (!_conf.Init(conf)) return false;

    return true;
}

bool Server::InitMsgPipe()
{
    if (_conf.msg_shmkey() == 0)
        return false;

    size_t shm_size = sizeof(ShmPipe) + MSG_PIPE_SIZE* sizeof(stMsg) + 1;
    bool exsit;

    // create or attach shm
    if ((exsit = ShareMem::IsExist(_conf.msg_shmkey())) == false)
        ShareMem::Create(_conf.msg_shmkey(), shm_size);
    ShareMem shm(_conf.msg_shmkey());

    if (!shm.IsAttached())
    {
        ShareMem::Delete(_conf.msg_shmkey());
        return false;
    }

    // create pipe
    ShmPipe::pshm = &shm;
    pmsg_pipe = new ShmPipe(exsit);

    if (!pmsg_pipe)
    {
        ShareMem::Delete(_conf.msg_shmkey());
        return false;
    }

    return true;
}

bool Server::InitCsPipe()
{
    if (_conf.cs_shmkey() == 0 || _conf.sc_shmkey() == 0) return true;

    size_t cs_size = sizeof(ShmPipe) + CS_PIPE_SIZE* TCP_BUF_LEN;
    size_t sc_size = sizeof(ShmPipe) + SC_PIPE_SIZE* TCP_BUF_LEN;
    bool cs_exist, sc_exist;

    // create or attach shm
    if ((cs_exist = ShareMem::IsExist(_conf.cs_shmkey())) == false)
        ShareMem::Create(_conf.cs_shmkey(), cs_size);
    ShareMem cs_shm(_conf.cs_shmkey());

    if ((sc_exist = ShareMem::IsExist(_conf.sc_shmkey())) == false)
        ShareMem::Create(_conf.sc_shmkey(), sc_size);
    ShareMem sc_shm(_conf.sc_shmkey());

    if (!cs_shm.IsAttached() || !sc_shm.IsAttached())
    {
        ShareMem::Delete(_conf.cs_shmkey());
        ShareMem::Delete(_conf.sc_shmkey());
        return false;
    }

    // create pipe
    ShmPipe::pshm = &cs_shm;
    pcs_pipe = new ShmPipe(cs_exist);
    ShmPipe::pshm = &sc_shm;
    psc_pipe = new ShmPipe(sc_exist);

    if (!pcs_pipe || !psc_pipe)
    {
        ShareMem::Delete(_conf.cs_shmkey());
        ShareMem::Delete(_conf.sc_shmkey());
        return false;
    }

    return true;
}

bool Server::InitCenter()
{
    if (_conf.center_addrs_size() == 0)
        return false;
    if (_conf.center_addrs_size() > MAX_CENTER_NUM)
        return false;

    _center_num = _conf.center_addrs_size();
    for (uint32 i = 0; i < _center_num; ++i)
    {
        _center_mng[i].Init(_conf.center_addrs(i), _info);

        if (!_center_mng[i].Connect())
            return false;

        if (!_center_mng[i].Register())
            return false;
    }

    return true;
}

int Server::Initialize(int argc, char* argv[], uint32 type)
{
    _info.svr_type = type;
    // args
    char conf[32] = {0};
    ParseArgs(argc, argv, conf);

    // load conf
    if (!LoadConfig(conf)) return -1;

    // msg queue
    if (!InitMsgPipe()) return -2;

    // connect center
    if (!InitCenter()) return -5;

    // init cs/sc shm
    if (!InitCsPipe()) return -3;

    // logic init
    if (!InitLogic()) return -4;

    // signal
    signal(SIGRTMIN, SignalExit);
    signal(SIGRTMIN+1, SignalReload);

    return 0;
}

void Server::Run()
{
    while (1)
    {
        if (CheckRunFlag() != 0)
            break;

        SetTimeMsec();      // update time
        CheckCenterConn();  // 检查center连接
        GetCenterMessage();    // 获取center消息
        GetClientMessage();    // 获取cspipe消息
        DispatchMessage();  // 分派消息
        OnRoutineCheck();   // logic routine
    }
}

int Server::CheckRunFlag()
{
    if (run_flag & EN_Flag_Exit)
    {
        Exit();
        return -1;
    }
    if (run_flag & EN_Flag_Reload)
    {
        Reload();
        run_flag &= ~EN_Flag_Reload;
    }
    return 0;
}

void Server::CheckCenterConn()
{
    // 每秒一次check
    static uint32 check_time = GetTime();
    if (check_time + 1 <= GetTime())
    {
        check_time = GetTime();
        for (uint32 i = 0; i < _center_num; ++i)
        {
            _center_mng[i].CheckConnect();
        }
    }
}

void Server::GetClientMessage()
{
    if (!pcs_pipe) return;

    for (uint32 i = 0; i < 1024; ++i)
    {
        _tcp_len = TCP_BUF_LEN;
        if (!pcs_pipe->GetOneData(&_tcp_buf[0], _tcp_len)) return;  //sc empty

        if (_tcp_len <= sizeof(stTcpHead) + 4) continue;
        _tcp_len -= TCPHEAD_LEN;
        stTcpHead* phead = (stTcpHead*)_tcp_buf;
        if (phead->msg_type == CLI_DISCON)
        {
            if (_tcp_len != sizeof(uint64)) continue;

            uint64 index = *(uint64*)(_tcp_buf + TCPHEAD_LEN);
            OnClientOffline(index);
        }
        else
        {
            if (_tcp_len <= sizeof(uint32)) continue;
            uint32 pb_len = ntohl(*(uint32*)(_tcp_buf + TCPHEAD_LEN));
            if (pb_len != _tcp_len) continue;

            if (!PostMessage(*phead, _tcp_buf + TCPHEAD_LEN + sizeof(uint32), _tcp_len - sizeof(uint32)))
                return;
        }
    }
}

void Server::GetCenterMessage()
{
    // select
    fd_set fds;
    FD_ZERO(&fds);
    for (uint32 i = 0; i < _center_num; ++i)
        _center_mng[i].AddFd(fds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;  // 5ms
    int fd_num = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
    if (fd_num <= 0) return;

    // recv data
    for (uint32 i = 0; i < _center_num; ++i)
    {
        if (!_center_mng[i].HasInput(fds)) continue;

        _center_mng[i].RecvData();
    }

    // post to msg queue
    uint32 count = 0;
    for (uint32 i = 0; i < _center_num; ++i)
    {
        if (count >= 512) return;

        while (_center_mng[i].GetOneData(_center_buf, sizeof(_center_buf)))
        {
            uint32 len = *(uint32*)_center_buf;
            if (len <= 4 + sizeof(stCenterHead)) continue;

            stCenterHead* phead = (stCenterHead*)(_center_buf + 4);
            char* pb_buf = _center_buf + 4 + sizeof(stCenterHead);
            uint32 pb_len = len - 4 - sizeof(stCenterHead);

            if (!PostMessage(*phead, pb_buf, pb_len))
                return;
            count++;
        }
    }
}

bool Server::PostMessage(stTcpHead& head, char* buf, uint32 buf_len)
{
    _msg.type = EN_MSG_CLIENT;
    _msg.msg_head.tcp_head = head;
    _msg.pb_msg.data_len = buf_len;
    memcpy(&_msg.pb_msg.data, buf, buf_len);

    return pmsg_pipe->AppendOneData((const char*)&_msg, sizeof(_msg));
}

bool Server::PostMessage(stCenterHead& head, char* buf, uint32 buf_len)
{
    _msg.type = EN_MSG_SERVER;
    _msg.msg_head.center_head = head;
    _msg.pb_msg.data_len = buf_len;
    memcpy(&_msg.pb_msg.data, buf, buf_len);

    return pmsg_pipe->AppendOneData((const char*)&_msg, sizeof(_msg));
}

bool Server::PostMessage(char* buf, uint32 buf_len)
{
    _msg.type = EN_MSG_INTERNAL;
    _msg.pb_msg.data_len = buf_len;
    memcpy(&_msg.pb_msg.data, buf, buf_len);

    return pmsg_pipe->AppendOneData((const char*)&_msg, sizeof(_msg));
}

void Server::DispatchMessage()
{
    for (uint32 i = 0; i < 1024; ++i)
    {
        _msg_len = sizeof(_msg);
        if (!pmsg_pipe->GetOneData((char*)&_msg, _msg_len)) return;
        if (_msg_len != sizeof(_msg)) continue;

        if (_msg.type == EN_MSG_CLIENT)
            OnClientMessage(_msg.msg_head.tcp_head, _msg.pb_msg);

        else if (_msg.type == EN_MSG_SERVER)
            OnServerMessage(_msg.msg_head.center_head, _msg.pb_msg);

        else if (_msg.type == EN_MSG_INTERNAL)
            OnInteralMessage(_msg.pb_msg);

        else;
            // unknown type
    }
}

bool Server::SendMessageToInternal(char* buf, uint32 len)
{
    return PostMessage(buf, len);
}

bool Server::SendMessageToClient(stTcpHead& head, char* buf, uint32 len)
{
    if (!psc_pipe) return false;

    _tcp_len = 0;
    // tcp 头
    memcpy(&_tcp_buf[_tcp_len], &head, sizeof(head));
    _tcp_len += sizeof(head);

    // 包长
    uint32 total_len = htonl(len + sizeof(uint32));
    memcpy(&_tcp_buf[_tcp_len], &total_len, sizeof(total_len));
    _tcp_len += sizeof(total_len);

    // 数据
    memcpy(&_tcp_buf[_tcp_len], buf, len);
    _tcp_len += len;

    return psc_pipe->AppendOneData(_tcp_buf, _tcp_len);
}

bool Server::SendMessageToCenter(stCenterHead& head, char* buf, uint32 len, int center_index)
{
    static uint32 send_count = 0;
    if (center_index == -1)
    {
        switch (head.route_type)
        {
            case route_hash:    // 以hash key路由
                center_index = head.hash_key % _center_num;
                break;
            case route_p2p:
            case route_broadcast:
                center_index = send_count++ % _center_num;
                break;
            default: break;
        }
    }

    if (center_index == -1 || center_index >= _center_num)
        return false;
    head.center_index = center_index;

    _center_len = 0;
    // 包长
    uint32 total_len = 4 + sizeof(head) + len;
    memcpy(&_center_buf[_center_len], &total_len, sizeof(total_len));
    _center_len += sizeof(total_len);
    // center 头
    memcpy(&_center_buf[_center_len], &head, sizeof(head));
    _center_len += sizeof(head);
    // 数据
    memcpy(&_center_buf[_center_len], buf, len);
    _center_len += len;

    return _center_mng[center_index].SendOneData(_center_buf, _center_len);
}

bool Server::NotifyTcpLogicIndex(stTcpHead& head, uint64 logic_index)
{
    if (!psc_pipe) return false;

    _tcp_len = 0;
    uint32 len = sizeof(logic_index);

    // tcp 头
    memcpy(&_tcp_buf[_tcp_len], &head, sizeof(head));
    stTcpHead* phead = (stTcpHead*)_tcp_buf;
    phead->msg_type = LOGIC_INDEX;
    _tcp_len += sizeof(head);

    // 数据
    memcpy(&_tcp_buf[_tcp_len], &logic_index, len);
    _tcp_len += len;

    return psc_pipe->AppendOneData(_tcp_buf, _tcp_len);
}


