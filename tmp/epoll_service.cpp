/*
 * epoll_service.cpp
 *
 *  Created on: 2017年3月7日
 *      Author: chenhuapan
 */

#include "epoll_service.h"

CEpoll::CEpoll():
	m_iEfd(-1),
	m_iMaxFd(0),
	m_pstEvents(NULL)
{

}
CEpoll::~CEpoll() {
	// TODO Auto-generated destructor stub
	Close();
}

bool CEpoll::Create(int maxFd)
{
	Close();

	if(maxFd <= 0)return false;

	m_iMaxFd = maxFd;

	m_pstEvents = new epoll_event[m_iMaxFd];
	m_iEfd = epoll_create(m_iMaxFd);

	if(m_iMaxFd < 0)
	{
		printf("failed to create epoll!![error:%s]", strerror(errno));
		return false;
	}
	return true;
}

void CEpoll::Close()
{
	if(m_iEfd > 0)
	{
		close(m_iEfd);
		m_iEfd = 0;
	}

	if(m_pstEvents)
	{
		delete[] m_pstEvents;
		m_pstEvents = NULL;
	}
}

int CEpoll::Wait(int timeOutMs, int fdNum)
{
	if(-1==fdNum) fdNum = m_iMaxFd;
	return epoll_wait(m_iEfd, m_pstEvents, m_iMaxFd, timeOutMs);
}

bool CEpoll::Add(int fd, void* ptr, uint32_t events)
{
	struct epoll_event stEv;
	bzero(&stEv, sizeof(struct epoll_event) );
	if(ptr)
	{
		stEv.data.ptr = ptr;
	}
	else
	{
		stEv.data.fd = fd;
	}

	stEv.events = events;

	if(epoll_ctl(m_iEfd, EPOLL_CTL_ADD, fd, &stEv) < 0)
	{
		return false;
	}

	return true;
}

bool CEpoll::Mod(int fd, void* ptr, uint32_t events)
{
	struct epoll_event stEv;
	if(ptr)
	{
		stEv.data.ptr = ptr;
	}
	else
	{
		stEv.data.fd = fd;
	}

	stEv.events = events;

	if(epoll_ctl(m_iEfd, EPOLL_CTL_MOD, fd, &stEv) < 0)
	{
		return false;
	}

	return true;
}

bool CEpoll::Del(int fd)
{
	struct epoll_event stEv;

	if(fd < 0)return false;

	if(epoll_ctl(m_iEfd, EPOLL_CTL_DEL, fd, &stEv) < 0)
	{
		printf("Epoll delete fd(%d) failed!![error:%s]", fd, strerror(errno) );
		return false;
	}

	return true;
}
