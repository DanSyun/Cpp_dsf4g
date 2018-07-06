#include "player_mng.h"
#include "serialize.hpp"

size_t Player::GetByteSize()
{
    //pb len + pb bytes
    return sizeof(size_t) + base::ByteSize() + sizeof(_tcp_head);
}

bool Player::Serialize(uint8* buf, size_t size, size_t& offset)
{
    if (!SerializePB(buf, size, offset, *this))         return false;
    if (!SerializeBase(buf, size, offset, _tcp_head))   return false;

    return true;
}

bool Player::UnSerialize(uint8* buf, size_t size, size_t& offset)
{
    if (!UnSerializePB(buf, size, offset, *this))       return false;
    if (!UnSerializeBase(buf, size, offset, _tcp_head)) return false;

    return true;
}

bool PlayerManager::Init(uint32 size, uint32 shmkey)
{
    if (!base::Init(size, shmkey)) return false;

    // ÖØ½¨Ë÷Òı
    for (auto iter = base::Begin(); iter != base::End(); ++iter)
    {
        _index[iter->uid()] = iter.GetPos();
    }

    return true;
}

Player* PlayerManager::CreatePlayer(uint64 uid)
{
    int32 pos = base::Malloc();
    if (pos == -1)
        return NULL;

    Player* pplayer = base::Get(pos);
    pplayer->set_uid(uid);

    _index[uid] = pos;

    return pplayer;
}

Player* PlayerManager::GetPlayer(uint64 uid)
{
    if (_index.find(uid) == _index.end())
        return NULL;

    return base::Get(_index[uid]);
}

bool PlayerManager::ReleasePlayer(uint64 uid)
{
    if (_index.find(uid) == _index.end())
        return false;

    base::Free(_index[uid]);
    _index.erase(uid);

    return true;
}

