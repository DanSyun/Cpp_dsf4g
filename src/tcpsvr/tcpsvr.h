#pragma once

#include <fcntl.h>
#include <list>

#include "base.h"
#include "config.h"
#include "shm_pipe.h"
#include "share_mem.hpp"



#define LISTEN_SOCKET   1
#define CLINET_SOCKET   2

#define RECV_BUF_LEN    (TCPHEAD_LEN + MAX_BUF_LEN* 100)   // at least 100 package

// tcp socket
struct stSocket
{
    int     fd;
    int     type;
    uint64  logic_index;    // logic对象索引，通常是uid
    char*   recv_buf;
    uint32  buf_offset;
    uint64  create_time;
    uint32  socket_pos;
    uint32  socket_port;
    char    socket_addr[16];
};


class TcpSvr
{
public:
    void ParseArgs(int argc, char* argv[], char* conf);
    bool LoadConfig(const char* conf);
    bool InitNetwork();
    bool InitShmPipe();
    int Initialize(int argc, char* argv[]);

    void Run();
    int CheckRunFlag();
    void OnRoutineCheck();
    void DealNetwork();
    void AcceptConnect();
    void RecvClientData(stSocket* psocket);
    void SendClientData();
    void SendDataToClient();
    void NotifyLogicDiscon(const stSocket& socket);

    bool SocketPoolInit();
    stSocket* SocketCreate();
    stSocket* SocketGet(int pos);
    void SocketRelease(int pos, bool notify_logic = true);

    bool EpollInit();
    bool EpollAdd(int fd, int pos);
    bool EpollDel(int fd);

private:
    TcpConfig _conf;

    int _lsfd;
    stSocket* _pool;
    char* _all_recv_buf;
    std::list<uint32> _free_list;

    char _send_buf[TCP_BUF_LEN];
    uint32 _send_len;

    int _epfd;
    struct epoll_event _event;
    struct epoll_event* _events;

    ShareMem* pcs_shm;
    ShmPipe* pcs_pipe;
    ShareMem* psc_shm;
    ShmPipe* psc_pipe;
};

