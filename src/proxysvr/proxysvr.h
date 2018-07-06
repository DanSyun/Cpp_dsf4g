#pragma once

#include <fcntl.h>
#include <list>
#include <unordered_map>
#include <vector>

#include "base.h"
#include "config.h"

#define LISTEN_SOCKET   1
#define CLINET_SOCKET   2

#define RECV_BUF_LEN    (CENTER_BUF_LEN* 100)   // at least 100 package
#define SEND_BUF_LEN    (CENTER_BUF_LEN* 100)
#define SOCK_CLOSED     (-1)

// tcp socket
struct stSvrSocket
{
    int     fd;
    int     type;
    char*   recv_buf;
    uint32  recv_offset;
    uint32  socket_pos;
    uint32  svr_type;
    uint32  svr_id;
    char    send_buf[SEND_BUF_LEN];
    uint32  send_offset;
};

class SvrManager;
class ProxySvr
{
public:
    void ParseArgs(int argc, char* argv[], char* conf);
    bool LoadConfig(const char* conf);
    bool InitNetwork();
    int Initialize(int argc, char* argv[]);

    void Run();
    int CheckRunFlag();
    void OnRoutineCheck();
    void DealNetwork();
    void AcceptConnect();
    void RecvServerData(stSvrSocket* psocket);

    bool SocketPoolInit();
    stSvrSocket* SocketCreate();
    stSvrSocket* SocketGet(int pos);
    void SocketRelease(int pos);
    void SocketDisconn(int pos);

    bool EpollInit();
    bool EpollAdd(int fd, int pos);
    bool EpollDel(int fd);

    CenterConfig& GetConf() { return _conf; }
private:
    CenterConfig _conf;

    int _lsfd;
    stSvrSocket* _pool;
    char* _all_recv_buf;
    std::list<uint32> _free_list;

    int _epfd;
    struct epoll_event _event;
    struct epoll_event* _events;

    SvrManager* _psvr_mng;
};



class SvrManager
{
public:
    void Init(ProxySvr* pserver);
    void Register(stSvrSocket& socket);
    void DoTransfer(const char* buf, uint32 len);


    void ReserveData(stSvrSocket& socket, const char* buf, uint32 len);
    bool SendBufData(stSvrSocket& socket);
    bool SendOneData(stSvrSocket& socket, const char* buf, uint32 len);
private:
    // key: svr_type, value: svrs(index: svr_id)
    std::unordered_map<uint32, std::vector<stSvrSocket*> >  _svrs;
    ProxySvr* _pserver;
};

