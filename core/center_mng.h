#pragma once

#include <string>
#include "base.h"

#define RECV_BUF_LEN    (CENTER_BUF_LEN* 100)   // at least 100 package
#define SEND_BUF_LEN    (CENTER_BUF_LEN* 100)
#define SOCK_CLOSED     (-1)

using namespace std;

class CenterMng
{
public:
    void Init(const PBAddr& addr, const stSvrInfo& info);
    bool Connect();
    bool Register();
    void CheckConnect();

    void ReserveData(const char* buf, uint32 len);
    bool SendBufData();
    bool SendOneData(const char* buf, uint32 len);
    void RecvData();
    bool GetOneData(char* buf, uint32 buf_len);

    void AddFd(fd_set& fds);
    bool HasInput(const fd_set& fds);
private:
    uint32  _fd;
    char    _recv_buf[RECV_BUF_LEN];
    uint32  _recv_offset;
    char    _send_buf[SEND_BUF_LEN];
    uint32  _send_offset;

    string  _ip;
    uint32  _port;
    stSvrInfo   _svr_info;
};

