/*
 * fake_random.h
 *
 *  Created on: 2017年3月8日
 *      Author: chenhuapan
 */

#ifndef CORE_FAKE_RANDOM_H_
#define CORE_FAKE_RANDOM_H_

#include "common_include.h"
#include "singleton.hpp"
#include "type_def.h"

class CFakeRandom : public TSingleton<CFakeRandom>
{
public:
	CFakeRandom() : m_uiSeed( 0 )
	{ }

	~CFakeRandom() {}

	void SetSeed( uint32_t uiSeed ) { m_uiSeed = uiSeed; }

	//随机一个整数
	uint32_t Random();

	// 随机数范围[0, uiRange - 1]
	uint32_t Random( uint32_t uiRange );

	// 随机数范围[uiMinRange, uiMaxRange - 1]
	uint32_t Random( uint32_t uiMinRange, uint32_t uiMaxRange );

	//随机抽奖
	bool Lottery( float fRate, uint32_t uiAccuracy = 1000 );

private:
	uint32_t m_uiSeed;
};

extern CFakeRandom* RandomIns;

#endif /* CORE_FAKE_RANDOM_H_ */
