#include "timer_mng.h"
#include "msg_basic.h"
#include "serialize.hpp"

size_t Timer::GetByteSize()
{
    return sizeof(size_t) + base::ByteSize();
}

bool Timer::Serialize(uint8* buf, size_t size, size_t& offset)
{
    if (!SerializePB(buf, size, offset, *this))         return false;

    return true;
}

bool Timer::UnSerialize(uint8* buf, size_t size, size_t& offset)
{
    if (!UnSerializePB(buf, size, offset, *this))       return false;

    return true;
}

bool TimerManager::Init(uint32 size, uint32 shmkey)
{
    if (!base::Init(size, shmkey)) return false;

    // session id 从0开始
    _cur_max_id = 0;

    // 重建索引
    for (auto iter = base::Begin(); iter != base::End(); ++iter)
    {
        uint32 timer_id = iter->timer_id();
        _index[timer_id] = iter.GetPos();
        if (timer_id > _cur_max_id)
            _cur_max_id = timer_id;
    }

    return true;
}

void TimerManager::Tick()
{
    std::vector<uint32> ids;
    for (auto iter = base::Begin(); iter != base::End(); ++iter)
    {
        if (iter->timeout_msec() <= GetTimeMsec())
        {
            MsgBasic::SendMsgToInternal(iter->msg());
            ids.push_back(iter->timer_id());
        }
    }
    for (auto iter = ids.begin(); iter != ids.end(); ++iter)
    {
        ReleaseTimer(*iter);
    }
}

int TimerManager::CreateTimer(uint32 msec, const PBInterMsg& inter)
{
    int32 pos = base::Malloc();
    if (pos == -1)
        return -1;

    // 生成timer_id
    uint32 timer_id;
    uint32 count = 0;
    while (true)
    {
        timer_id = ++_cur_max_id;
        if (_index.find(timer_id) == _index.end())
            break;
        count++;
        if (count >= 10)
            return -1;
    }
    _cur_max_id = _cur_max_id & 0xfffffff;

    Timer* ptimer = base::Get(pos);
    ptimer->set_timer_id(timer_id);
    ptimer->set_timeout_msec(GetTimeMsec() + msec);
    ptimer->mutable_msg()->CopyFrom(inter);

    _index[timer_id] = pos;

    return timer_id;
}

bool TimerManager::ReleaseTimer(uint32 timer_id)
{
    if (_index.find(timer_id) == _index.end())
        return false;

    base::Free(_index[timer_id]);
    _index.erase(timer_id);

    return true;
}

