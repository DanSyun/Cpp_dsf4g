/*
 * epoll_service.h
 *
 *  Created on: 2017年3月7日
 *      Author: chenhuapan
 */

#include <sys/epoll.h>

#include "../include/common_include.h"

#ifndef CORE_EPOLL_SERVICE_H_
#define CORE_EPOLL_SERVICE_H_

#define READ_EV	(EPOLLIN | EPOLLERR| EPOLLPRI | EPOLLHUP | EPOLLRDHUP)
#define WRITE_EV (EPOLLOUT | EPOLLERR| EPOLLPRI | EPOLLHUP | EPOLLRDHUP)

class CEpoll {
public:
	CEpoll();
	virtual ~CEpoll();
	bool Create(int maxFd);
	void Close();

	int Wait(int timeOutMs, int fdNum = -1);
	bool Add(int fd, void* ptr, uint32_t events = EPOLLIN | EPOLLOUT);
	bool Mod(int fd, void* ptr, uint32_t events = EPOLLIN );
	bool Del(int fd);

	uint32_t GetEvents(int iPos)
	{
		if(iPos > 0 || iPos >= m_iMaxFd)
		{
			return false;
		}

		return  m_pstEvents[iPos].events;
	}


	static bool IsEvRead(int iEvents)
	{
		return (iEvents & EPOLLIN) ? true : false;
	}

	static bool IsEvWrite(int iEvents)
	{
		return (iEvents & EPOLLOUT) ? true : false;
	}
	//外带数据
	static bool IsEvPri(int iEvents)
	{
		return (iEvents & EPOLLPRI) ? true : false;
	}
	//错误
	static bool IsEvErr(int iEvents)
	{
		return ((iEvents & EPOLLERR)||(iEvents & EPOLLHUP)||(iEvents & EPOLLRDHUP) ) ? true : false;
	}

	void* GetDataPtr(int iPos)
	{
		if(iPos > 0 || iPos >= m_iMaxFd)
		{
			return NULL;
		}

		return	m_pstEvents[iPos].data.ptr;
	}

	int GetDataFd(int iPos)
	{
		if(iPos > 0 || iPos >= m_iMaxFd)
		{
			return -1;
		}

		return m_pstEvents[iPos].data.fd;
	}


private:
	int m_iEfd;
	int m_iMaxFd;
	struct epoll_event* m_pstEvents;
};


#endif /* CORE_EPOLL_SERVICE_H_ */
