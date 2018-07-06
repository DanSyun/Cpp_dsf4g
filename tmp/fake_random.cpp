/*
 * fake_random.cpp
 *
 *  Created on: 2017年3月8日
 *      Author: chenhuapan
 */

#include "fake_random.h"

CFakeRandom* RandomIns = CFakeRandom::Instance();

uint32_t CFakeRandom::Random()
{
    unsigned int next = m_uiSeed;
    unsigned int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    m_uiSeed = next;

    return result;
}


uint32_t CFakeRandom::Random( uint32_t uiRange )
{
    if( 0 == uiRange )
    {
        return 0;
    }

    return Random() % uiRange;
}


uint32_t CFakeRandom::Random( uint32_t uiMinRange, uint32_t uiMaxRange )
{
    if( uiMinRange == uiMaxRange )
    {
        return uiMinRange;
    }

    if( uiMinRange > uiMaxRange )
    {
        return Random( uiMinRange - uiMaxRange ) + uiMaxRange;
    }
    else
    {
        return Random( uiMaxRange - uiMinRange ) + uiMinRange;
    }
}

bool CFakeRandom::Lottery( float fRate, uint32_t uiAccuracy/*=1000*/ )
{
    if ( fRate <= 0.0 )
        return false;

    if ( fRate >= 1.0 )
        return true;

    uint32_t uiRateVal = (uint32_t)( fRate*uiAccuracy );
    uint32_t uiRandomVal =  Random( uiAccuracy );

    if ( uiRandomVal < uiRateVal )
        return true;
    else
        return false;
}

