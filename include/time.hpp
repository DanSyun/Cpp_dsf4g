#ifndef _INC_TIME_H_
#define __INC_TIME_H_
#pragma once

#include <time.h>
#include <algorithm>
#include <sys/time.h>
#include "type_def.h"

#define SECS_PER_DAY    (60 *60 *24)

inline uint32 GetDiffDays(time_t t1, time_t t2)
{
    if (t1 > t2) std::swap(t1, t2);

    uint32 count = (t2 - t1) /SECS_PER_DAY;
    time_t tmp = t1 + count *SECS_PER_DAY;

    struct tm st1, st2;
    localtime_r(&tmp, &st1);
    localtime_r(&t2, &st2);

    return count + st2.tm_yday - st1.tm_yday;
}

inline void GetCurTime(uint32& sec, uint32& usec)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0)
    {
        sec = tv.tv_sec;
        usec = tv.tv_usec;
    }
}

#endif
