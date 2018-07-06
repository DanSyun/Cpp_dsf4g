#include "base.h"
#include "time.hpp"

uint64 g_cur_time = SetTimeMsec();

uint64 SetTimeMsec()
{
    uint32 sec, usec;
    GetCurTime(sec, usec);
    g_cur_time = (uint64)sec* 1000 + usec/ 1000;
    return g_cur_time;
}

uint64 GetTimeMsec()
{
	return g_cur_time;
}

uint32 GetTime()
{
	return g_cur_time/ 1000;
}

