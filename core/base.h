#pragma once

#include "type_def.h"

// tcp logic间通信协议
#define TCP_NORMAL  0    // 正常cs消息
#define LOGIC_INDEX 1    // logic通知tcp建立索引
#define CLI_DISCON  2    // tcp通知logic断开

#define TCPHEAD_LEN     sizeof(stTcpHead)
#define CENTERHEAD_LEN  sizeof(stCenterHead)
#define CS_PIPE_SIZE    1024
#define SC_PIPE_SIZE    4096
#define MSG_PIPE_SIZE   2048    // 1024 cs msg, 512 ss msg, 512 inter msg
#define MAX_BUF_LEN     4096                            // 最大包长
#define MAX_PB_LEN      (MAX_BUF_LEN - sizeof(uint32))  // pb最大字节数
#define TCP_BUF_LEN     (TCPHEAD_LEN + MAX_BUF_LEN)     // tcp logic间最大包长
#define CENTER_BUF_LEN  (CENTERHEAD_LEN + MAX_BUF_LEN)  // logic center间最大包长
//#define	MSG_SIZE		(sizeof(stMsg))
//#define MSG_BUF_LEN 	(MSG_SIZE+sizeof(uint32))

struct stSvrInfo
{
    uint32  svr_type;
    uint32  svr_id;
};

// 客户端消息结构 pb_len + pb_buf
// tcp logic间消息结构 total_len + stTcpHead + pb_len + pb_buf
struct stTcpHead
{
    uint64  create_time;
    uint32  socket_pos;
    uint32  socket_port;
    char    socket_addr[16];
    uint32  msg_type;       // LOGIC_INDEX通知tcp建立logic索引，CLI_DISCON通知logic断开
    bool    need_close;     // 逻辑主动关闭连接
};

// logic center间消息结构 total_len + stCenterHead + pb_buf
enum RouteType
{
    route_p2p   = 1,
    route_hash  = 2,
    //route_random    = 3,
    route_broadcast = 4,
};
struct stCenterHead
{
    uint32  src_type;
    uint32  src_id;
    uint32  des_type;
    uint32  des_id;
    uint32  session_id;
    RouteType   route_type;
    uint64  hash_key;

    uint32  center_index; // 原路返回
};

// 消息队列对象结构
enum EN_MSG_TYPE
{
    EN_MSG_CLIENT   = 1,
    EN_MSG_SERVER   = 2,
    EN_MSG_INTERNAL = 3,
};
union uMsgHead
{
    stTcpHead       tcp_head;
    stCenterHead    center_head;
};
struct stPBMsg
{
    uint32  data_len;
    char    data[MAX_PB_LEN];
};
struct stMsg
{
    EN_MSG_TYPE type;
    stPBMsg     pb_msg;
    uMsgHead    msg_head;
};

// auth token结构
struct stAuthToken
{
    uint64  uid;
    uint32  time;
};

extern uint64 SetTimeMsec();
extern uint64 GetTimeMsec();
extern uint32 GetTime();

