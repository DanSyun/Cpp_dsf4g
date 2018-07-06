#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcpsvr.h"
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

void TcpSvr::ParseArgs(int argc, char* argv[], char* conf)
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

bool TcpSvr::LoadConfig(const char* conf)
{
    if (!conf)
        return false;
    if (!_conf.Init(conf))
        return false;

    return true;
}

bool TcpSvr::InitNetwork()
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
    stSocket* psocket = SocketCreate();
    psocket->type = LISTEN_SOCKET;

    // epoll
    if (!EpollInit()) return false;
    if (!EpollAdd(_lsfd, psocket->socket_pos)) return false;

    return true;
}

bool TcpSvr::SocketPoolInit()
{
    uint32 client_num = _conf.max_client_num();
    if (client_num > _free_list.max_size()) return false;

    _pool = new stSocket[client_num];
    _all_recv_buf = new char[client_num* RECV_BUF_LEN];
    for (uint32 pos = 0; pos < client_num; ++pos)
    {
        _pool[pos].recv_buf = _all_recv_buf + pos* RECV_BUF_LEN + TCPHEAD_LEN;
        _pool[pos].socket_pos = pos;
        _free_list.push_back(pos);
    }
    return true;
}

stSocket* TcpSvr::SocketCreate()
{
    if (_free_list.empty())
        return NULL;

    uint32 pos = _free_list.front();
    _free_list.pop_front();

    return &_pool[pos];
}

void TcpSvr::SocketRelease(int pos, bool notify_logic)
{
    if (pos >= _conf.max_client_num())
        return;

    stSocket& socket = _pool[pos];
    // close
    close(socket.fd);
    // del from ep
    EpollDel(socket.fd);
    // notify logic client close
    if (notify_logic && socket.logic_index != 0)
        NotifyLogicDiscon(socket);

    // clear
    memset(&socket, 0, sizeof(socket));
    socket.recv_buf = _all_recv_buf + pos* RECV_BUF_LEN + TCPHEAD_LEN;
    socket.socket_pos = pos;

    _free_list.push_back(pos);
}

stSocket* TcpSvr::SocketGet(int pos)
{
    if (pos >= _conf.max_client_num())
        return NULL;

    return &_pool[pos];
}

bool TcpSvr::EpollInit()
{
    memset(&_event, 0, sizeof(struct epoll_event));
    _event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    _event.data.ptr = NULL;
    _event.data.fd  = -1;

    uint32 client_num = _conf.max_client_num();
    _events = (struct epoll_event*)malloc(client_num* sizeof(struct epoll_event));
    if ((_epfd = epoll_create(client_num)) == -1)
        return false;

    return true;
}

bool TcpSvr::EpollAdd(int fd, int pos)
{
    _event.data.fd = pos;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_event) == -1)
        return false;

    return true;
}

bool TcpSvr::EpollDel(int fd)
{
    return epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1? false: true;
}

bool TcpSvr::InitShmPipe()
{
    if (_conf.cs_shmkey() == 0 || _conf.sc_shmkey() == 0) return false;

    size_t cs_size = sizeof(ShmPipe) + CS_PIPE_SIZE* TCP_BUF_LEN;
    size_t sc_size = sizeof(ShmPipe) + SC_PIPE_SIZE* TCP_BUF_LEN;
    bool cs_exist, sc_exist;

    // create or attach shm
    if ((cs_exist = ShareMem::IsExist(_conf.cs_shmkey())) == false)
        ShareMem::Create(_conf.cs_shmkey(), cs_size);
    pcs_shm = new ShareMem(_conf.cs_shmkey());

    if ((sc_exist = ShareMem::IsExist(_conf.sc_shmkey())) == false)
        ShareMem::Create(_conf.sc_shmkey(), sc_size);
    psc_shm = new ShareMem(_conf.sc_shmkey());

    if (!pcs_shm->IsAttached() || !psc_shm->IsAttached())
    {
        if (pcs_shm->IsAttached())
            ShareMem::Delete(_conf.cs_shmkey());
        else if (psc_shm->IsAttached())
            ShareMem::Delete(_conf.sc_shmkey());
        return false;
    }

    // create pipe
    ShmPipe::pshm = pcs_shm;
    pcs_pipe = new ShmPipe(cs_exist);
    ShmPipe::pshm = psc_shm;
    psc_pipe = new ShmPipe(sc_exist);

    if (!pcs_pipe || !psc_pipe)
    {
        ShareMem::Delete(_conf.cs_shmkey());
        ShareMem::Delete(_conf.sc_shmkey());
        return false;
    }

    return true;
}

int TcpSvr::Initialize(int argc, char* argv [])
{
    // args
    char conf[32] = {0};
    ParseArgs(argc, argv, conf);

    // init log

    // load conf
    if (!LoadConfig(conf)) return -1;

    // net
    if (!InitNetwork()) return -2;

    // init cs/sc shm
    if (!InitShmPipe()) return -3;

    // signal
    signal(SIGRTMIN, SignalExit);

    return 0;
}

void TcpSvr::Run()
{
    while (1)
    {
        if (CheckRunFlag() != 0)
            break;

        OnRoutineCheck();
        DealNetwork();
        SendClientData();
    }
}

int TcpSvr::CheckRunFlag()
{
    if (run_flag & EN_Flag_Exit)
    {
        pcs_shm->Detach();
        psc_shm->Detach();
        return -1;
    }
    return 0;
}

void TcpSvr::OnRoutineCheck()
{
    //SetTimeMsec();
}

void TcpSvr::DealNetwork()
{
    int fdnum = epoll_wait(_epfd, _events, _conf.max_client_num(), 10);
    if (fdnum == -1) return;

    int i;
    struct epoll_event *events;
    for (i = 0, events = _events; i < fdnum; i++, events++)
    {
        int pos = events->data.fd;
        stSocket* psocket = SocketGet(pos);
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
            RecvClientData(psocket);
        }
    }
}

void TcpSvr::AcceptConnect()
{
    stSocket* psocket = SocketCreate();
    if (!psocket) return;

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int clifd = accept(_lsfd, (struct sockaddr*)&addr, &addr_len);
    if (clifd == -1)
        return SocketRelease(psocket->socket_pos);

    int flag;
    if ((flag = fcntl(clifd, F_GETFL, 0)) == -1 || fcntl(clifd, F_SETFL, flag| O_NONBLOCK) == -1)
        return SocketRelease(psocket->socket_pos);
    if (!EpollAdd(clifd, psocket->socket_pos))
        return SocketRelease(psocket->socket_pos);

    uint32 sec, usec;
    GetCurTime(sec, usec);
    psocket->fd = clifd;
    psocket->type = CLINET_SOCKET;
    psocket->create_time = (uint64)sec* 1000 + usec/ 1000;
    psocket->logic_index = 0;
    psocket->buf_offset = 0;
    psocket->socket_port = ntohs(addr.sin_port);
    strcpy(psocket->socket_addr, inet_ntoa(addr.sin_addr));
}

void TcpSvr::RecvClientData(stSocket* psocket)
{
    int recv_len;
    do
    {
        if ((RECV_BUF_LEN - TCPHEAD_LEN) == psocket->buf_offset)
            return;

        recv_len = recv(psocket->fd, psocket->recv_buf + psocket->buf_offset,
            RECV_BUF_LEN - TCPHEAD_LEN - psocket->buf_offset, 0);

        if (recv_len == 0)
            return SocketRelease(psocket->socket_pos);

        if (recv_len == -1)
            if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                return SocketRelease(psocket->socket_pos);

        psocket->buf_offset += recv_len;
        uint32 left = psocket->buf_offset;
        char* tmp_recv = psocket->recv_buf;
        char* tmp_pipe = psocket->recv_buf - TCPHEAD_LEN; // start from tcphead
        while (left > 4)
        {
            uint32 len = ntohl(*(int*)tmp_recv);
            if (len > MAX_BUF_LEN || len < 4)   // len invalid
                return SocketRelease(psocket->socket_pos);

            if (len > left) break; // left not enough

            stTcpHead* phead = (stTcpHead*)tmp_pipe;
            phead->socket_pos = psocket->socket_pos;
            phead->create_time = psocket->create_time;
            phead->socket_port = psocket->socket_port;
            phead->msg_type = TCP_NORMAL;
            strcpy(phead->socket_addr, psocket->socket_addr);

            if (!pcs_pipe->AppendOneData(tmp_pipe, len + TCPHEAD_LEN)) break; // cs full

            left -= len;
            tmp_recv += len;
            tmp_pipe += len;
        }
        if (left != psocket->buf_offset)
        {
            memmove(psocket->recv_buf, tmp_recv, left);
            psocket->buf_offset = left;
        }
    } while (recv_len > 0);
}

void TcpSvr::SendClientData()
{
    for (uint32 i = 0; i < 512; ++i)
    {
        _send_len = TCP_BUF_LEN;
        if (!psc_pipe->GetOneData(&_send_buf[0], _send_len)) break;  //sc err

        SendDataToClient();
    }
}

void TcpSvr::SendDataToClient()
{
    stTcpHead* phead = (stTcpHead*)_send_buf;
    stSocket* psocket = SocketGet(phead->socket_pos);
    if (!psocket) return;

    if (phead->create_time != psocket->create_time)
        return; // reuse socket

    // skip tcphead
    _send_len -= TCPHEAD_LEN;

    if (phead->msg_type == LOGIC_INDEX)
    {
        if (_send_len == sizeof(uint64))
            psocket->logic_index = *(uint64*)(_send_buf + TCPHEAD_LEN);
        return;
    }
    // block send
    while (true)
    {
        int len = send(psocket->fd, _send_buf + TCPHEAD_LEN, _send_len, 0);
        if (len == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)  // try again
                continue;
            else    // failed
                return SocketRelease(psocket->socket_pos);
        }
        else    // succ
        {
            if (phead->need_close) // close
            {
                SocketRelease(psocket->socket_pos, false);
            }
            return;
        }
    }
}

void TcpSvr::NotifyLogicDiscon(const stSocket& socket)
{
    char* tmp_recv = socket.recv_buf;
    char* tmp_pipe = socket.recv_buf - TCPHEAD_LEN; // start from tcphead

    stTcpHead* phead = (stTcpHead*)tmp_pipe;
    phead->msg_type = CLI_DISCON;
    memcpy(tmp_recv, &socket.logic_index, sizeof(socket.logic_index));

    pcs_pipe->AppendOneData(tmp_pipe, sizeof(socket.logic_index) + TCPHEAD_LEN);
}

int main(int argc, char* argv[])
{
    TcpSvr* psvr = new TcpSvr;
    if (psvr->Initialize(argc, argv) == 0)
        psvr->Run();
    return 0;
}

