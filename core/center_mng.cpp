#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "center_mng.h"

void CenterMng::Init(const PBAddr& addr, const stSvrInfo& info)
{
    _ip = addr.ip();
    _port = addr.port();
    _svr_info = info;

    _recv_offset = 0;
    _send_offset = 0;
}

bool CenterMng::Connect()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return false;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_ip.c_str());
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) return false;

    uint32 buf_size = 102400;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) == -1) return false;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) == -1) return false;

    int flag = -1;
    if ((flag = fcntl(fd, F_GETFL, 0)) == -1)
        return false;
    if (fcntl(fd, F_SETFL, flag| O_NONBLOCK) == -1)
        return false;

    _fd = fd;

    return true;
}

bool CenterMng::Register()
{
    while (true)
    {
        int len = send(_fd, (char*)&_svr_info, sizeof(_svr_info), 0);
        if (len == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)  // try again
                continue;
            else    // failed
            {
                return false;
            }
        }
        else    // succ
        {
            return true;
        }
    }
}

void CenterMng::CheckConnect()
{
    if (_fd != SOCK_CLOSED)
        return;

    if (!Connect())
        return;

    if (!Register())
        return;

    SendBufData();
}

void CenterMng::ReserveData(const char* buf, uint32 len)
{
    // 如果满了就丢掉防止雪崩
    // 或者动态扩展缓存
    if (_send_offset + len > sizeof(_send_buf))
        _send_offset = 0;

    memcpy(&_send_buf[_send_offset], buf, len);
    _send_offset += len;
}

bool CenterMng::SendBufData()
{
    int ret = send(_fd, _send_buf, _send_offset, 0);
    if (ret == -1)
    {
        if (!(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            close(_fd);
            _fd = SOCK_CLOSED;
            _recv_offset = 0;
        }
        return false;
    }
    else
    {
        _send_offset = 0;
        return true;
    }
}

bool CenterMng::SendOneData(const char * buf, uint32 len)
{
    // 如果已断开直接缓存等待重连
    if (_fd == SOCK_CLOSED)
    {
        ReserveData(buf, len);
        return false;
    }

    // 先发送缓存
    if (_send_offset != 0)
    {
        if (!SendBufData())
        {
            // 发送失败则直接缓存本次数据
            ReserveData(buf, len);
            return false;
        }
    }

    // 再发送本次数据
    int ret = send(_fd, buf, len, 0);
    if (ret == -1)
    {
        if (!(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            close(_fd);
            _fd = SOCK_CLOSED;
            _recv_offset = 0;
        }
        ReserveData(buf, len);
        return false;
    }

    return true;
}

void CenterMng::RecvData()
{
    int recv_len;
    do
    {
        if (sizeof(_recv_buf) == _recv_offset) return;
        recv_len = recv(_fd, _recv_buf + _recv_offset, sizeof(_recv_buf) - _recv_offset, 0);
        if (recv_len == 0)
        {
            close(_fd);
            _fd = SOCK_CLOSED;
            _recv_offset = 0;
            return;
        }
        if (recv_len == -1)
        {
            if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
            {
                close(_fd);
                _fd = SOCK_CLOSED;
                _recv_offset = 0;
            }
            return;
        }
        _recv_offset += recv_len;
    } while (recv_len > 0);
}

bool CenterMng::GetOneData(char* buf, uint32 buf_len)
{
    if (_recv_offset <= 4) return false;

    uint32 len = *(uint32*)_recv_buf;
    if (len > _recv_offset) return false;
    if (len > buf_len)
    {
        close(_fd);
        _fd = SOCK_CLOSED;
        _recv_offset = 0;
        return false;
    }

    memcpy(buf, _recv_buf, len);
    _recv_offset -= len;

    if (_recv_offset != 0)
    {
        memmove(_recv_buf, _recv_buf + len, _recv_offset);
    }
    return true;
}

void CenterMng::AddFd(fd_set& fds)
{
    if (_fd != SOCK_CLOSED)
        FD_SET(_fd, &fds);
}

bool CenterMng::HasInput(const fd_set& fds)
{
    if (_fd == SOCK_CLOSED)
        return false;

	return FD_ISSET(_fd, &fds);
}

