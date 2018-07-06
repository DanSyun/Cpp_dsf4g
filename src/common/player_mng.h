#pragma once

#include <unordered_map>
#include "object_mng.hpp"
#include "singleton.hpp"
#include "pb_object.pb.h"
#include "base.h"

class Player: public PBPlayer
{
private:
    typedef PBPlayer base;
public:
    size_t GetByteSize();
    bool Serialize(uint8* buf, size_t size, size_t& offset);
    bool UnSerialize(uint8* buf, size_t size, size_t& offset);

    stTcpHead&  GetTcpHead()                        { return _tcp_head; }
    void        SetTcpHead(const stTcpHead& head)   { _tcp_head = head; }
private:
    stTcpHead _tcp_head;    // 实现消息推送
};

class PlayerManager:
    public ObjectMng<Player>,
    public Singleton<PlayerManager>
{
public:
    typedef ObjectMng<Player> base;
    typedef std::unordered_map<uint64, uint32> index;
public:
    bool Init(uint32 size, uint32 shmkey = 0);

    Player* CreatePlayer(uint64 uid);
    Player* GetPlayer(uint64 uid);
    bool ReleasePlayer(uint64 uid);

private:
    index _index;
};

