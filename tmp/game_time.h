/*
 * game_time.h
 *
 *  Created on: 2017年3月8日
 *      Author: chenhuapan
 */

#ifndef CORE_GAME_TIME_H_
#define CORE_GAME_TIME_H_

#define MS_PER_SECOND   1000
#define SECONDS_OF_DAY  86400

#include <string>
#include "common_include.h"
#include "singleton.hpp"


//游戏时间单例

class CGameTime : public TSingleton<CGameTime>
{
public:
	CGameTime();
    ~CGameTime() {};

	void UpdateTime();

	struct timeval* GetCurrTime( ){ return &m_stCurrTv; }
	time_t	GetCurrSecond( ) { return m_stCurrTv.tv_sec; }

	//ms
	uint16_t GetCurrMsInSec() { return (uint16_t)( m_stCurrTv.tv_usec / 1000 ); }

	time_t GetCurrMiliSec( )
	{
		UpdateTime();
		return 1000*m_stCurrTv.tv_sec + m_stCurrTv.tv_usec/1000;
	}

	int GetCurrHour() { return m_stCurrTm.tm_hour; }
	int GetCurrYear(){ return m_stCurrTm.tm_year; }
	int GetCurrMonth(){ return m_stCurrTm.tm_mon; }
	int GetCurrDay(){ return m_stCurrTm.tm_mday; }
	int GetCurrMin(){ return m_stCurrTm.tm_min; }
	int GetCurrSec(){ return m_stCurrTm.tm_sec; }
	const struct tm* GetCurrTm(){ return &m_stCurrTm; }
	const struct timeval* GetCurrTv(){	return &m_stCurrTv; }

	//时间转化
	time_t mkTime(tm* brokentime){return mktime(brokentime); }

	//跨天
	bool IsDayAlternate( time_t lNowTime, time_t lLastTime );

	//判断是否过了SecPass 秒，默认一天
	bool IsPassSeconds(time_t lNowTime, time_t lLastTime, time_t SecPass = SECONDS_OF_DAY);

    //到明天零点还有多少秒
    time_t GetSecToNextDayBreak();

	// 获得当天开始时间基准点
	time_t GetBeginOfDay() { return m_lBegineOfDay; }

	bool IsInSameWeek( time_t lTime1, time_t lTime2 );

	tm* GetSysTime();
	uint32_t GetCurDayOfWeek();
	bool IsContain(struct tm* curTm, struct tm* startTm,  struct tm* endTm);

	int GetWeekNum(time_t lTime){return _GetWeekNum(lTime);}
		    // 时间转换成字符串
		    // @param stTm: 时间结构
		    // @param sFormat: 格式
		    // @return string 时间字符串
	std::string tm2str(const struct tm &stTm, const std::string &sFormat = "%Y-%m-%d %H:%M:%S");

		 // 判断是否为同一月
		 // lNowTime, lLastTime为秒
	bool IsSameMonth( time_t lNowTime, time_t lLastTime );
private:
	void _SetUtcTimeOff();
	int _GetWeekNum( time_t lTime );

private:
	struct timeval 	m_stCurrTv;
	struct tm      	m_stCurrTm;
	time_t		   	m_lBegineOfDay; // 当天的基准时间( utc )
	int				m_iUtcOff; // 和UTC时区之间的差值
	time_t m_Time;
    tm* m_pTm;
};

extern CGameTime* GameTimeIns;

#endif /* CORE_GAME_TIME_H_ */
