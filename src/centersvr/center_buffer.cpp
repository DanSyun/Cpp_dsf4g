/*
 * byte_buffer.cpp
 *
 *  Created on: 2017年3月11日
 *      Author: chenhuapan
 */

#include "center_buffer.h"

#include "common_include.h"
#include <iostream>

template<>
bool CBuffer<uint8_t>::ReallocBuffer(uint32_t size)
{
	if(m_readPos > 0)
	{
		uint32_t tmpSize = m_writePos-m_readPos;
		memmove(m_buffer, m_buffer+m_readPos, tmpSize);
		m_writePos -= m_readPos;
		m_readPos = 0;
		tmpSize = m_curSzie-(sizeof(uint8_t))-m_writePos;
		size = (size > tmpSize) ?  (size - tmpSize) : 0;
	}

	if(size > 0)
	{
		if(size+m_curSzie > m_maxSize){
			std::cout<<"memory lacking"<<std::endl;
			return false;
		}
		size = (m_realcSize > size) ? m_realcSize : size;
		m_curSzie += size;
		m_buffer = reinterpret_cast<uint8_t*>(realloc(m_buffer, m_curSzie));
	}

	return true;
}



