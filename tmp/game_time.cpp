/*
 * game_time.cpp
 *
 *  Created on: 2017年3月8日
 *      Author: chenhuapan
 */

#include <sys/time.h>
#include "singleton.hpp"
#include <time.h>
#include <math.h>
#include "common_include.h"
#include "game_time.h"

enum WEEK_DAY
{
	SUNDAY,
    MONDAY,
    TUESDAY,
    WENDNESDAY,
    THURDDAY,
    FRIDAY,
    SATURDAY,
};

CGameTime *GameTimeIns = CGameTime::Instance();

CGameTime::CGameTime():
	m_lBegineOfDay(0),
	m_iUtcOff(0),
	m_Time(0),
	m_pTm(NULL)
{
    bzero( &m_stCurrTv, sizeof(m_stCurrTv));
    bzero( &m_stCurrTm, sizeof(m_stCurrTm) );
    this->_SetUtcTimeOff();
}


void CGameTime::UpdateTime()
{
    time_t lLastTime = m_stCurrTv.tv_sec;

    gettimeofday(&m_stCurrTv, NULL);
    localtime_r(&m_stCurrTv.tv_sec, &m_stCurrTm);

    if( lLastTime != m_stCurrTv.tv_sec &&
        this->IsDayAlternate(m_stCurrTv.tv_sec, lLastTime) )
    {
        struct tm tTime = {0};
        tTime.tm_year = m_stCurrTm.tm_year;
        tTime.tm_mon = m_stCurrTm.tm_mon;
        tTime.tm_mday = m_stCurrTm.tm_mday;
        m_lBegineOfDay = mktime(&tTime);
    }
}

bool CGameTime::IsDayAlternate( time_t lNowTime, time_t lLastTime )
{
    if( 0 == lNowTime )
    {
        return false;
    }

    if( 0 == lLastTime )
    {
        return true;
    }

    if( lNowTime == lLastTime )
    {
        return false;
    }

    uint8_t bNowDay = 0,bLastDay = 0;
    struct tm stNowTime;
    localtime_r(&lNowTime, &stNowTime);
    bNowDay = stNowTime.tm_mday;

    struct tm stLastTime;
    localtime_r(&lLastTime, &stLastTime);
    bLastDay = stLastTime.tm_mday;

    if( ( bNowDay == bLastDay ) && ( abs( lNowTime - lLastTime ) < SECONDS_OF_DAY ))
    {
        return false;
    }

    return true;
}

bool CGameTime::IsPassSeconds(time_t lNowTime, time_t lLastTime, time_t SecPass)
{
    if( 0 == lNowTime )
    {
        return false;
    }

    if( 0 == lLastTime )
    {
        return true;
    }

    if( lNowTime == lLastTime )
    {
        return false;
    }

    uint8_t bNowDay = 0,bLastDay = 0;
    struct tm stNowTime;
    localtime_r(&lNowTime, &stNowTime);
    bNowDay = stNowTime.tm_mday;

    struct tm stLastTime;
    localtime_r(&lLastTime, &stLastTime);
    bLastDay = stLastTime.tm_mday;

    if( ( bNowDay == bLastDay ) && ( abs( lNowTime - lLastTime ) < SecPass ))
    {
        return false;
    }

    return true;
}


time_t CGameTime::GetSecToNextDayBreak()
{
    struct tm tmNew;
    memcpy(&tmNew, &m_stCurrTm, sizeof(tmNew));

    tmNew.tm_hour = 0;
    tmNew.tm_min = 0;
    tmNew.tm_sec = 0;

    time_t tNew = mktime(&tmNew);
    tNew += SECONDS_OF_DAY;

    return (tNew - m_stCurrTv.tv_sec);
}


void CGameTime::_SetUtcTimeOff()
{
    time_t lNow = 0;
	time( &lNow );
	struct tm tmCurr = {0};
	tmCurr    = *localtime( &lNow );
	m_iUtcOff = tmCurr.tm_gmtoff;
}

uint32_t CGameTime::GetCurDayOfWeek()
{
    uint64_t tempTime = this->GetCurrSecond() + m_iUtcOff;
    tempTime /= SECONDS_OF_DAY;
    tempTime = (tempTime+4)%7;
    return tempTime;
}

int CGameTime::_GetWeekNum( time_t lTime )
{
    long lWeekNum = lTime + m_iUtcOff;
    lWeekNum /= SECONDS_OF_DAY;
    lWeekNum += 4;
    lWeekNum /= 7;
    lWeekNum += 1;
    return (int)lWeekNum;
}


bool CGameTime::IsInSameWeek( time_t lTime1, time_t lTime2 )
{
    int iWeek1 = this->_GetWeekNum( lTime1 );
    int iWeek2 = this->_GetWeekNum( lTime2 );
    return ( iWeek1 == iWeek2 );
}

tm* CGameTime::GetSysTime()
{
    time(&m_Time);
    m_pTm = localtime(&m_Time);
    return m_pTm;
}

bool CGameTime::IsContain(struct tm* curTm, struct tm* startTm,  struct tm* endTm)
{
	time_t curTime = mktime(curTm);
	time_t startTime = mktime(startTm);
	time_t endTime = mktime(endTm);
	if((curTime > startTime) && (curTime < endTime))
	{
		return true;
	}

	return false;
}

std::string CGameTime::tm2str(const struct tm &stTm, const std::string &sFormat)
{
    char sTimeString[255] = "\0";

    strftime(sTimeString, sizeof(sTimeString), sFormat.c_str(), &stTm);

    return std::string(sTimeString);
}


bool CGameTime::IsSameMonth( time_t lNowTime, time_t lLastTime )
{
    if( 0 == lNowTime )
    {
        return false;
    }

    if( 0 == lLastTime )
    {
        return false;
    }

    if( lNowTime == lLastTime )
    {
        return true;
    }

    uint8_t bNowMonth = 0,bLastMonth = 0;
    struct tm stNowTime;
    localtime_r(&lNowTime, &stNowTime);
    bNowMonth = stNowTime.tm_mon;

    struct tm stLastTime;
    localtime_r(&lLastTime, &stLastTime);
    bLastMonth = stLastTime.tm_mon;

    if(  bNowMonth == bLastMonth )
    {
        return true;
    }

    return false;
}
