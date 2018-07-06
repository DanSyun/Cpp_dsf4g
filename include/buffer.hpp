/*
 * buffer.hpp
 *
 *  Created on: 2017年3月7日
 *      Author: Administrator
 */

#ifndef INCLUDE_BUFFER_HPP_
#define INCLUDE_BUFFER_HPP_

#include "common_include.h"

#define _DEFAULT_BNUFFER_LEN_  10*1024
#define _DEFAULT_REALLOC_NUM_	1000

template<typename T>
class CBuffer{
public:
	CBuffer();
	~CBuffer();
public:
	//重置
	void Reset();

	void SetMaxSize(uint32_t maxNum){m_maxSize = maxNum*sizeof(T);}
	void SetReallocSize(uint32_t reallocNum){m_realcSize = reallocNum*sizeof(T);}


	//获取读buf
	T* ReadBuf();
	//获取读buf并滑动位置
	T* ReadBuf(uint32_t size);
	//读地址滑动
	bool ReadSlip(uint32_t size);
	//是否可读
	bool CanRead(uint32_t size);
	//可读数据长度
	int ReadSize(){return m_writePos-m_readPos;};

	//获取写buf
	T* WriteBuf();
	//获取写buf并滑动位置
	T* WriteBuf(uint32_t size);
	//按大小写入buf
	bool WriteBuf(const T* buffer, const uint32_t size);
	//写地址滑动
	bool WriteSlip(uint32_t size);
	//是否可写
	bool CanWrite(uint32_t size);

protected:
	bool ReallocBuffer(uint32_t size);
private:
	T* m_buffer;
	//写偏移地址
	uint32_t m_writePos;
	//读偏移地址
	uint32_t m_readPos;
	//新开辟大小
	uint32_t m_realcSize;
	//当前大小
	uint32_t m_curSzie;
	//最大
	uint32_t m_maxSize;
};

template<typename T>
CBuffer<T>::CBuffer():
	m_buffer(NULL),
	m_writePos(0),
	m_readPos(0),
	m_realcSize(_DEFAULT_REALLOC_NUM_*sizeof(T)),
	m_curSzie(0),
	m_maxSize(_DEFAULT_BNUFFER_LEN_*sizeof(T))
{

}

template<typename T>
CBuffer<T>::~CBuffer()
{
	if(NULL != m_buffer)
	{
		free(m_buffer);
	}
	m_buffer = NULL;
	m_writePos = 0;
	m_readPos = 0;
	m_maxSize = 0;
}

template<typename T>
void CBuffer<T>::Reset()
{
	m_readPos = 0;
	m_writePos = 0;
}

template<typename T>
bool CBuffer<T>::ReallocBuffer(uint32_t size)
{
	if(m_readPos > 0)
	{
		uint32_t tmpSize = m_writePos-m_readPos;
		memmove(m_buffer, m_buffer+m_readPos, tmpSize);
		m_writePos -= m_readPos;
		m_readPos = 0;
		tmpSize = m_curSzie-(sizeof(T))-m_writePos;
		size = (size >= tmpSize) ?  (size - tmpSize) : 0;
	}

	if(size > 0)
	{
		if(size+m_curSzie > m_maxSize){
			return false;
		}
		size = (m_realcSize > size) ? m_realcSize : size;
		m_curSzie += size;
		m_buffer = reinterpret_cast<T*>(realloc(m_buffer, m_curSzie));
	}

	return true;
}


template<typename T>
T* CBuffer<T>::ReadBuf()
{
	return (T*)(&m_buffer[m_readPos]);
}

template<typename T>
T* CBuffer<T>::ReadBuf(uint32_t size)
{
	T* pTmpBuf = (T*)(&m_buffer[m_readPos]);
	pTmpBuf = (true == ReadSlip(size) )  ?  ( pTmpBuf ): ( NULL);
	return pTmpBuf;
}

template<typename T>
bool CBuffer<T>::ReadSlip(uint32_t size)
{
	if((m_readPos+size) > m_writePos)
	{
		return false;
	}

	m_readPos += size;

	return true;
}

template<typename T>
bool CBuffer<T>::CanRead(uint32_t size)
{
	return ((m_readPos+size) > m_writePos) ? false : true;
}

template<typename T>
T* CBuffer<T>::WriteBuf()
{
	return (T*)(&m_buffer[m_writePos]);
}

template<typename T>
T* CBuffer<T>::WriteBuf(uint32_t size)
{
	T* pTmpBuff = (T*)(&m_buffer[m_writePos]);
	pTmpBuff = (true == WriteSlip(size)) ? pTmpBuff : NULL;

	return pTmpBuff;
}

template <typename T>
bool CBuffer<T>::WriteBuf(const T* buffer, uint32_t size)
{
	if(false == CanWrite(size) ) return false;
	bcopy(buffer, m_buffer+m_writePos, size);
	return WriteSlip(size);
}

template<typename T>
bool CBuffer<T>::WriteSlip(uint32_t size)
{
	if(false == CanWrite(size)) return false;

	m_writePos += size;

	return true;
}

template<typename T>
bool CBuffer<T>::CanWrite(uint32_t size)
{
	if(m_curSzie <= sizeof(T) || m_writePos+size >= (m_curSzie - sizeof(T)) )
	{
		if(false == ReallocBuffer(size))
		{
			return false;
		}
	}
	return true;
}


#endif /* INCLUDE_BUFFER_HPP_ */
