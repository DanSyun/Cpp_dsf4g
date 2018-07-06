#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

#include "proxysvr.h"
#include "time.hpp"
#include "daemon.hpp"


int run_flag = 0;
enum ENRunFlag
{
    EN_Flag_Exit = 1,
};
void SignalExit(int sig)
{
    run_flag |= EN_Flag_Exit;
}

void ProxySvr::ParseArgs(int argc, char* argv[], char* conf)
{
    int opt;
    while ((opt = getopt(argc, argv, "dc:")) != -1)
    {
        switch (opt)
        {
            case 'd': Daemonize(); break;
            case 'c': strcpy(conf, optarg); break;
            default: break;
        }
    }
}

bool ProxySvr::LoadConfig(const char* conf)
{
    if (!conf)
        return false;
    if (!_conf.Init(conf))
        return false;

    return true;
}

bool ProxySvr::InitNetwork()
{
    // socket
    _lsfd = socket(AF_INET, SOCK_STREAM |SOCK_NONBLOCK, 0);
    if (_lsfd == -1) return false;

    // socket options
    int opt = 1;
    struct linger ling = {0, 0};
    socklen_t buf_size = 102400;
    if (setsockopt(_lsfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) return false;
    if (setsockopt(_lsfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1) return false;
    if (setsockopt(_lsfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) == -1) return false;
    if (setsockopt(_lsfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) == -1) return false;
    if (setsockopt(_lsfd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) == -1) return false;

    // bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_conf.port());
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(_lsfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) return false;

    // listen
    if (listen(_lsfd, 1024) == -1) return false;

    // socket pool
    if (!SocketPoolInit()) return false;
    stSvrSocket* psocket = SocketCreate();
    psocket->type = LISTEN_SOCKET;

    // epoll
    if (!EpollInit()) return false;
    if (!EpollAdd(_lsfd, psocket->socket_pos)) return false;

    return true;
}

bool ProxySvr::SocketPoolInit()
{
    uint32 svr_num = _conf.max_svr_num();
    if (svr_num > _free_list.max_size()) return false;

    _pool = new stSvrSocket[svr_num];
    _all_recv_buf = new char[svr_num* RECV_BUF_LEN];
    for (uint32 pos = 0; pos < svr_num; ++pos)
    {
        _pool[pos].recv_buf = _all_recv_buf + pos* RECV_BUF_LEN;
        _pool[pos].socket_pos = pos;
        _free_list.push_back(pos);
    }
    return true;
}

stSvrSocket* ProxySvr::SocketCreate()
{
    if (_free_list.empty())
        return NULL;

    uint32 pos = _free_list.front();
    _free_list.pop_front();

    return &_pool[pos];
}

stSvrSocket* ProxySvr::SocketGet(int pos)
{
    if (pos >= _conf.max_svr_num())
        return NULL;

    return &_pool[pos];
}

void ProxySvr::SocketRelease(int pos)
{
    if (pos >= _conf.max_svr_num())
        return;

    stSvrSocket& socket = _pool[pos];
    // close
    close(socket.fd);
    // del from ep
    EpollDel(socket.fd);

    // clear
    memset(&socket, 0, sizeof(socket));
    socket.recv_buf = _all_recv_buf + pos* RECV_BUF_LEN;
    socket.socket_pos = pos;

    _free_list.push_back(pos);
}

void ProxySvr::SocketDisconn(int pos)
{
    if (pos >= _conf.max_svr_num())
        return;

    stSvrSocket& socket = _pool[pos];
    // close
    close(socket.fd);
    // del from ep
    EpollDel(socket.fd);

    socket.fd = SOCK_CLOSED;
}

bool ProxySvr::EpollInit()
{
    memset(&_event, 0, sizeof(struct epoll_event));
    _event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    _event.data.ptr = NULL;
    _event.data.fd  = -1;

    uint32 svr_num = _conf.max_svr_num();
    _events = (struct epoll_event*)malloc(svr_num* sizeof(struct epoll_event));
    if ((_epfd = epoll_create(svr_num)) == -1)
        return false;

    return true;
}

bool ProxySvr::EpollAdd(int fd, int pos)
{
    _event.data.fd = pos;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_event) == -1)
        return false;

    return true;
}

bool ProxySvr::EpollDel(int fd)
{
    return epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1? false: true;
}

int ProxySvr::Initialize(int argc, char* argv [])
{
    // args
    char conf[32] = {0};
    ParseArgs(argc, argv, conf);

    // init log

    // load conf
    if (!LoadConfig(conf)) return -1;

    // net
    if (!InitNetwork()) return -2;

    // svr mng
    _psvr_mng = new SvrManager;
    _psvr_mng->Init(this);

    // signal
    signal(SIGRTMIN, SignalExit);

    return 0;
}

void ProxySvr::Run()
{
    while (1)
    {
        if (CheckRunFlag() != 0)
            break;

        OnRoutineCheck();
        DealNetwork();
    }
}

int ProxySvr::CheckRunFlag()
{
    if (run_flag & EN_Flag_Exit)
    {
        return -1;
    }
    return 0;
}

void ProxySvr::OnRoutineCheck()
{
    //SetTimeMsec();
}

void ProxySvr::DealNetwork()
{
    int fdnum = epoll_wait(_epfd, _events, _conf.max_svr_num(), _conf.ep_timeout());
    if (fdnum == -1) return;

    int i;
    struct epoll_event *events;
    for (i = 0, events = _events; i < fdnum; i++, events++)
    {
        int pos = events->data.fd;
        stSvrSocket* psocket = SocketGet(pos);
        if (!psocket) continue;

        if ((EPOLLERR & events->events) != 0)
        {
            SocketRelease(pos);
            continue;
        }
        if ((EPOLLIN & events->events) == 0) continue;
        // accept conn
        if (psocket->type == LISTEN_SOCKET)
        {
            AcceptConnect();
        }
        // recv data
        else
        {
            RecvServerData(psocket);
        }
    }
}

void ProxySvr::AcceptConnect()
{
    stSvrSocket* psocket = SocketCreate();
    if (!psocket) return;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int clifd = accept(_lsfd, (struct sockaddr*)&addr, &addr_len);
    if (clifd == -1)
    {
        SocketRelease(psocket->socket_pos);
        return;
    }
    psocket->fd = clifd;
    psocket->type = CLINET_SOCKET;

    // 阻塞接收注册消息
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(clifd, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;  // 5ms
    int fd_num = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
    if (fd_num <= 0)
    {
        SocketRelease(psocket->socket_pos);
        return;
    }
    stSvrInfo svr_info;
    int len = recv(clifd, (char*)&svr_info, sizeof(svr_info), 0);
    if (len != sizeof(svr_info))
    {
        SocketRelease(psocket->socket_pos);
        return;
    }
    // 设置非阻塞，加入ep
    int flag;
    if ((flag = fcntl(clifd, F_GETFL, 0)) == -1 ||
        fcntl(clifd, F_SETFL, flag| O_NONBLOCK) == -1)
    {
        SocketRelease(psocket->socket_pos);
        return;
    }
    if (!EpollAdd(clifd, psocket->socket_pos))
    {
        SocketRelease(psocket->socket_pos);
        return;
    }

    // 利用svr_info建立索引
    psocket->svr_type = svr_info.svr_type;
    psocket->svr_id = svr_info.svr_id;
    _psvr_mng->Register(*psocket);
}

void ProxySvr::RecvServerData(stSvrSocket* psocket)
{
    int recv_len;
    do
    {
        if (RECV_BUF_LEN == psocket->recv_offset) return;
        recv_len = recv(psocket->fd, psocket->recv_buf + psocket->recv_offset, RECV_BUF_LEN - psocket->recv_offset, 0);
        if (recv_len == 0)
        {
            SocketDisconn(psocket->socket_pos);
            return;
        }
        if (recv_len == -1)
        {
            if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                SocketDisconn(psocket->socket_pos);
            return;
        }

        psocket->recv_offset += recv_len;
        uint32 left = psocket->recv_offset;
        char* tmp_recv = psocket->recv_buf;
        while (left > 4)
        {
            uint32 len = *(int*)tmp_recv;
            if (len > CENTER_BUF_LEN)   // len invalid
            {
                SocketRelease(psocket->socket_pos);
                break;
            }
            if (len > left) break; // left not enough

            _psvr_mng->DoTransfer(tmp_recv, len);

            left -= len;
            tmp_recv += len;
        }
        if (left != psocket->recv_offset)
        {
            memmove(psocket->recv_buf, tmp_recv, left);
            psocket->recv_offset = left;
        }
    } while (recv_len > 0);
}

void SvrManager::Init(ProxySvr * pserver)
{
    _pserver = pserver;
}

void SvrManager::Register(stSvrSocket& socket)
{
    // 生成svr数组
    if (_svrs.find(socket.svr_type) == _svrs.end())
    {
        const PBLogicSvr* psvr = _pserver->GetConf().GetSvr(socket.svr_type);
        if (!psvr) return;

        _svrs[socket.svr_type].resize(psvr->svr_num(), NULL);
    }
    // 如果是重连需要拷贝缓存，销毁之前socket
    stSvrSocket* pbefore = _svrs[socket.svr_type][socket.svr_id];
    if (pbefore)
    {
        memcpy(&socket.send_buf, &pbefore->send_buf, pbefore->send_offset);
        socket.send_offset = pbefore->send_offset;
        _pserver->SocketRelease(pbefore->socket_pos);

        SendBufData(socket);
    }
    _svrs[socket.svr_type][socket.svr_id] = &socket;
    std::cout << "type: " << socket.svr_type << " id: " << socket.svr_id << " registered" << std::endl;
}

void SvrManager::DoTransfer(const char* buf, uint32 len)
{
    if (len <= sizeof(stCenterHead)) return;    // 

    stCenterHead* phead = (stCenterHead*)(buf + 4);
    if (_svrs.find(phead->des_type) == _svrs.end()) return;

    switch (phead->route_type)
    {
        case route_p2p:
        {
            if (_svrs[phead->des_type][phead->des_id] == NULL) return;
            SendOneData(*_svrs[phead->des_type][phead->des_id], buf, len);
            break;
        }
        case route_broadcast:
        {
            const PBLogicSvr* psvr = _pserver->GetConf().GetSvr(phead->des_type);
            if (psvr == NULL) return;
            for (uint32 i = 0; i < psvr->svr_num(); ++i)
            {
                if (_svrs[phead->des_type][i] == NULL) continue;
                SendOneData(*_svrs[phead->des_type][phead->des_id], buf, len);
            }
            break;
        }
        case route_hash:
        {
            const PBLogicSvr* psvr = _pserver->GetConf().GetSvr(phead->des_type);
            if (psvr == NULL) return;
            uint32 hash_id = phead->hash_key % psvr->svr_num();
            if (_svrs[phead->des_type][hash_id] == NULL) return;
            SendOneData(*_svrs[phead->des_type][hash_id], buf, len);
            break;
        }
        default: break;
    }
}

void SvrManager::ReserveData(stSvrSocket& socket, const char* buf, uint32 len)
{
    // 如果满了就丢掉防止雪崩
    // 或者动态扩展缓存
    if (socket.send_offset + len > sizeof(socket.send_buf))
        socket.send_offset = 0;

    memcpy(&socket.send_buf[socket.send_offset], buf, len);
    socket.send_offset += len;
}

bool SvrManager::SendBufData(stSvrSocket& socket)
{
    int ret = send(socket.fd, socket.send_buf, socket.send_offset, 0);
    if (ret == -1)
    {
        if (!(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            close(socket.fd);
            socket.fd = SOCK_CLOSED;
            socket.recv_offset = 0;
        }
        return false;
    }
    else
    {
        socket.send_offset = 0;
        return true;
    }
}

bool SvrManager::SendOneData(stSvrSocket& socket, const char* buf, uint32 len)
{
    // 如果已断开直接缓存等待重连
    if (socket.fd == SOCK_CLOSED)
    {
        ReserveData(socket, buf, len);
        return false;
    }

    // 先发送缓存
    if (socket.send_offset != 0)
    {
        if (!SendBufData(socket))
        {
            // 发送失败则直接缓存本次数据
            ReserveData(socket, buf, len);
            return false;
        }
    }

    // 再发送本次数据
    int ret = send(socket.fd, buf, len, 0);
    if (ret == -1)
    {
        if (!(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            close(socket.fd);
            socket.fd = SOCK_CLOSED;
            socket.recv_offset = 0;
        }
        ReserveData(socket, buf, len);
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    ProxySvr* psvr = new ProxySvr;
    if (psvr->Initialize(argc, argv) == 0)
        psvr->Run();
    return 0;
}

