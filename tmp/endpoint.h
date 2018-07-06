/*
 * endpoint.h
 *
 *  Created on: 2017年3月9日
 *      Author: chenhuapan
 */

#ifndef CORE_ENDPOINT_H_
#define CORE_ENDPOINT_H_

#include "common_include.h"
#include "net_util.hpp"

class CBaseEndpoint {
public:
	CBaseEndpoint();
	CBaseEndpoint(int sockfd);
	virtual ~CBaseEndpoint();
public:
	void SetNetAddr(net_addr_t addr){m_NetAddr = addr;}
	net_addr_t GetNetAddr(){return m_NetAddr;}

	void SetSockFd(int sockfd){m_Sockfd = sockfd;}
	int GetSockFd(){return m_Sockfd;}

	virtual int onReadEv(){return 0;}
	virtual int onWriteEv(){return 0;}

protected:
	//绑定套接字
	int m_Sockfd;
	//网络地址
	net_addr_t m_NetAddr;
};

#endif /* CORE_ENDPOINT_H_ */
