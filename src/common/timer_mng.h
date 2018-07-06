#pragma once

#include <unordered_map>
#include "object_mng.hpp"
#include "singleton.hpp"
#include "pb_object.pb.h"
#include "base.h"

class Timer: public PBTimer
{
private:
    typedef PBTimer base;
public:
    size_t GetByteSize();
    bool Serialize(uint8* buf, size_t size, size_t& offset);
    bool UnSerialize(uint8* buf, size_t size, size_t& offset);
};

class TimerManager:
    public ObjectMng<Timer>,
    public Singleton<TimerManager>
{
public:
    typedef ObjectMng<Timer> base;
    typedef std::unordered_map<uint32, uint32> index;
public:
    bool Init(uint32 size, uint32 shmkey = 0);
    void Tick();

    int CreateTimer(uint32 msec, const PBInterMsg& inter);
    bool ReleaseTimer(uint32 timer_id);

private:
    index   _index;
    uint32  _cur_max_id;
};


