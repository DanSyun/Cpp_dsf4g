#ifndef _NET_UTIL_H_
#define _NET_UTIL_H_

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "common_include.h"

/*=======================================================
 * define type
 *=======================================================
 *=======================================================*/
typedef enum sock_style
{
	sock_style_null = 0,
	sock_style_tcp = 1 << 0,
	sock_style_udp = 1 << 1,
}sock_style_t;

typedef struct net_addr{
	uint32_t	addr;
	uint16_t	port;
}net_addr_t;

enum select_mask{
	select_mask_read	= 1 << 0,
	select_mask_write 	= 1 << 1,
};

inline int net_tcp_sock()
{
	return socket(PF_INET, SOCK_STREAM, 0);
}

inline
int net_udp_sock()
{
	return socket(PF_INET, SOCK_DGRAM, 0);
}

inline
int net_close(int sockfd)
{
	return ::close(sockfd);
}

inline
bool  net_set_nonblocking(int sockfd)
{
	return fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0)|O_NONBLOCK );
}

inline
int net_bind(int sockfd, const char* pszAddr, uint32_t port)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof(sockaddr_in) );

	addr.sin_family = PF_INET;
	addr.sin_port = htons(port);

	if(pszAddr)
	{
		addr.sin_addr.s_addr = inet_addr(pszAddr);
	}
	else
	{
		addr.sin_addr.s_addr = INADDR_ANY;
	}

	int flag = 1;
	int len = sizeof(int);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);

	return bind(sockfd, (const sockaddr*)&addr, sizeof(sockaddr_in) );
}

inline
int net_listen(int sockfd, int backlog = 5)
{
	 return listen(sockfd, backlog);
}

inline
int net_connect(int sockfd, const char* pszAddr, uint32_t port)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof(sockaddr_in) );

	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr(pszAddr);
	addr.sin_port = htons(port);

	return connect(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in) );
}

inline
void net_set_buffsize(int sockfd, int nRcvBuf, int nSndBuf)
{
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const int*)&nRcvBuf, sizeof(int) );
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const int*)&nSndBuf, sizeof(int) );
}

inline
void net_get_buffsize(int sockfd, int* nRcvBuf, int* nSndBuf)
{
	socklen_t n1 = sizeof(int), n2 = sizeof(int);
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, nRcvBuf, &n1);
	getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, nSndBuf, &n2);
};

inline
int net_select_wait(int sockfd, int mask, int timeoutMS)
{
	int iResult = 0;

	struct timeval tv;
	struct timeval* ptv = NULL;
	if(timeoutMS > 0)
	{
		tv.tv_sec = timeoutMS/1000;
		tv.tv_usec = (timeoutMS%1000)*1000;
		ptv = &tv;
	}

	if(sockfd != 0)
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sockfd, &rfds);

		iResult = select(sockfd+1, (mask & select_mask_read ? &rfds : NULL),
				(mask & select_mask_write ? &rfds : NULL), NULL, ptv);
	}
	else
	{
		iResult = select(0, NULL, NULL, NULL, ptv);
	}

	return iResult;
}

inline
int net_async_read(int sockfd, void* buf, size_t bufsize,  int timeoutMS)
{
	int iRet = net_select_wait(sockfd, select_mask_read, timeoutMS);
	if(iRet > 0)
	{
		return ::read(sockfd, buf, bufsize);
	}

	return iRet;
}

inline
int net_tcp_accept(int listenSockFd,  net_addr_t& pSockAddr)
{
	sockaddr_in add_in;
	socklen_t socklen = sizeof(struct sockaddr);

	int acceptSockfd = -1;

	while(acceptSockfd < 0)
	{
		acceptSockfd = ::accept(listenSockFd, (sockaddr*)&add_in, &socklen);
		if(acceptSockfd < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return 0;
			}
			else if(errno == EINTR)
			{
				continue;
			}

			return -2;
		}
	}

	pSockAddr.addr = add_in.sin_addr.s_addr;
	pSockAddr.port = add_in.sin_port;

	return acceptSockfd;
}

inline
int net_tcp_send(int sockfd, const void* pBuf, uint64_t ulLen)
{
	if(0 >= ulLen)
	{
		return 0;
	}

	if(!pBuf)
	{
		return -1;
	}

	int iRet = -1;

	while(iRet < 0)
	{
		iRet = ::send(sockfd, pBuf, ulLen, 0);
		if(iRet < 0)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return 0;
			}
			else if(errno == EINTR)
			{
				continue;
			}
			else
			{
				return -2;
			}
		}
	}

	return iRet;
}

inline
int net_tcp_recv(int sockfd, void* pBuf, uint64_t ulLen)
{
	if(0 >= ulLen)
	{
		return 0;
	}

	if(!pBuf)
	{
		return -1;
	}

	int iRet = -1;
	while(iRet < 0)
	{
		iRet = ::recv(sockfd, pBuf, ulLen, 0);
		if(iRet < 0)
		{
			if(EAGAIN == errno || EWOULDBLOCK == errno)
			{
				return 0;
			}
			else if(EINTR == errno)
			{
				continue;
			}
			else
			{
				return -2;
			}
		}
	}

	return iRet;
}

inline
net_addr_t net_addr_aton(const char* pszAddr, uint32_t uiPort)
{
	net_addr_t stAddr;

	stAddr.addr = inet_addr(pszAddr);
	stAddr.port = htons(uiPort);

	return stAddr;
}

inline
void net_addr_ntoa(net_addr_t stAddr, char* pszAddr, uint32_t& uiPort)
{

	struct in_addr addr;
	addr.s_addr = stAddr.addr;
	strcpy(pszAddr , inet_ntoa(addr) );
	uiPort = ntohs(stAddr.port);
}

inline
int net_udp_sendto(int sockfd, const void* pBuf, uint64_t ulLen, net_addr_t tRemoteAddr)
{
	if(0 <= ulLen)
	{
		return 0;
	}

	if(!pBuf)
	{
		return -1;
	}

	if(tRemoteAddr.addr < 0 || tRemoteAddr.port < 0)
	{
		return -1;
	}

	sockaddr_in addr_in;
	bzero(&addr_in, sizeof(sockaddr_in) );
	addr_in.sin_port = tRemoteAddr.port;
	addr_in.sin_family = PF_INET;
	addr_in.sin_addr.s_addr = tRemoteAddr.addr;

	int iRet = -1;

	while(iRet < 0)
	{
		iRet = ::sendto(sockfd, pBuf, ulLen, 0, (sockaddr*)&addr_in, sizeof(sockaddr_in) );
		if(iRet < 0)
		{
			if(EAGAIN == errno || EWOULDBLOCK == errno)
			{
				return 0;
			}
			else if(EINTR == errno)
			{
				continue;
			}
			else
			{
				return -2;
			}
		}
	}

	return iRet;
}

inline
int net_udp_recvfrom(int sockfd, void* pBuf, uint64_t ulLen,  net_addr_t& tRemoteAddr)
{
	if(ulLen <= 0)
	{
		return 0;
	}

	if(!pBuf)
	{
		return -1;
	}

	if(tRemoteAddr.addr < 0 || tRemoteAddr.port < 0)
	{
		return -1;
	}

	sockaddr_in addr_in;
	bzero(&addr_in, sizeof(sockaddr_in) );
	addr_in.sin_family = PF_INET;

	int iRet = -1;
	while(0 > iRet)
	{
		socklen_t addrLen = sizeof(struct sockaddr);
		iRet = ::recvfrom(sockfd, pBuf, ulLen, 0, (sockaddr*)&addr_in, &addrLen);
		if(0 > iRet)
		{
			if(EAGAIN == errno || EWOULDBLOCK == errno)
			{
				return 0;
			}
			else if(EINTR == errno)
			{
				continue;
			}
			else
			{
				return -2;
			}
		}
	}

	tRemoteAddr.addr = addr_in.sin_addr.s_addr;
	tRemoteAddr.port = addr_in.sin_port;

	return iRet;
}

inline
int net_create_listen_sock(const char* pszAddr, uint32_t uiPort, int iBacklog, sock_style_t tStyle)
{
	int fd = -1;
	switch(tStyle)
	{
		case sock_style_tcp:
			fd = net_tcp_sock();
			break;
		case sock_style_udp:
			fd = net_udp_sock();
			break;
		default:
			break;
	}

	if(fd < 0)
	{
		return -1;
	}

	net_set_nonblocking(fd);

	enum{_BUFF_SIZE_ = 174760};
	net_set_buffsize(fd, _BUFF_SIZE_, _BUFF_SIZE_);

	int iRet = net_bind(fd, pszAddr, uiPort);
	if(0 != iRet)
	{
		printf("bind failed!![fd=%d, ipaddr=%s, port=%d]", fd, pszAddr, uiPort);
		return -1;
	}

	iRet = net_listen(fd, iBacklog);
	if(0 != iRet)
	{
		printf("listen failed!![fd=%d, ipaddr=%s, port=%d]", fd, pszAddr, uiPort);
		return -1;
	}

	return fd;
}


#endif /* COMMON_NET_UTIL_H_ */
