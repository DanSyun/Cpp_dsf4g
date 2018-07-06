#include <iostream>
#include <string>
#include "pb_cs_msg.pb.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "base.h"
#include <unistd.h>
#include "proto_log.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    ProtoLog::Instance()->Init();
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return 0;
    std::string ip = "127.0.0.1";
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) return 0;

    std::cout << "request autherization." << std::endl;
    PBCSMsg cs_msg;
    cs_msg.mutable_cs_request_auth()->set_type(EN_ACC_GEUST);
    cs_msg.mutable_cs_request_auth()->set_account("test");
    if (cs_msg.ByteSize() > MAX_PB_LEN) return 0;
    char buf[MAX_BUF_LEN];
    uint32 buf_len = cs_msg.ByteSize() + sizeof(uint32);

    cs_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)(buf + sizeof(uint32)));
    *(uint32*)buf = htonl(buf_len);

    while (true)
    {
        int len = send(fd, buf, buf_len, 0);
        if (len == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            else return 0;
        }
        else
            break;
    }
    uint64 uid = 0;
    string token;
    int len = recv(fd, buf, sizeof(buf), 0);
    if ((uint32)len > sizeof(uint32))
    {
        buf_len = ntohl(*(uint32*)buf);
        cs_msg.ParseFromArray(buf + sizeof(uint32), buf_len - sizeof(uint32));
        TraceProto(cs_msg);
        if (cs_msg.cs_response_auth().result() != EN_MESSAGE_OK)
            return 0;
        uid = cs_msg.cs_response_auth().uid();
        token = cs_msg.cs_response_auth().token();
    }
    close(fd);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return 0;
    ip = "127.0.0.1";
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9001);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) return 0;
    std::cout << "request login hall." << std::endl;
    srand(time(NULL));
    while (true)
    {
        PBCSMsg cs_msg;
        cs_msg.set_uid(uid);
        cs_msg.mutable_cs_request_login()->set_token(token);
        if (cs_msg.ByteSize() > MAX_PB_LEN) return 0;
        char buf[MAX_BUF_LEN];
        uint32 buf_len = cs_msg.ByteSize() + sizeof(uint32);

        cs_msg.SerializeWithCachedSizesToArray((google::protobuf::uint8*)(buf + sizeof(uint32)));
        *(uint32*)buf = htonl(buf_len);
        while (true)
        {
            int len = send(fd, buf, buf_len, 0);
            if (len == -1)
            {
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
                else return 0;
            }
            TraceProto(cs_msg);
            len = recv(fd, buf, sizeof(buf), 0);
            uint32 offset = 0;
            while ((uint32)len > sizeof(uint32))
            {
                buf_len = ntohl(*(uint32*)(buf + offset));
                cs_msg.ParseFromArray(buf + (offset + sizeof(uint32)), buf_len - sizeof(uint32));
                TraceProto(cs_msg);
                len -= buf_len;
                offset += buf_len;
            }
            usleep(2000000);
            break;
        }
    }
}
