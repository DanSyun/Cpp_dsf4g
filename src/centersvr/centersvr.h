#ifndef _CENTER_SVR_H_
#define _CENTER_SVR_H_

#include "common_include.h"
#include "time.hpp"
#include "center_endpoint.h"
#include "random.hpp"
#include "epoll_service.h"
#include <iostream>

class CCenterServer : public TSingleton<CCenterServer>
{
public:
	CCenterServer();
	virtual ~CCenterServer();
public:
	bool Initalize();
	void SetShutDown(bool bShutDown);
	void Run();
protected:
	bool CreateTcp();
	bool Update();

	int HandleNewTcp(int sockfd);
	bool HandleTcpRead(CCenterEndpoint* pEndpoint);
	bool HandleTcpWrite(CCenterEndpoint* pEndpoint);
private:
	bool ActiveWrite(CCenterEndpoint* pEndpoint);
	bool DeactiveWrite(CCenterEndpoint* pEndpoint);
	bool SendMsgToEndpoint(CCenterEndpoint* pEndpoint, uint8_t* pMsg, uint32_t size);

	bool DealCenterMsg(CCenterEndpoint* pEndpoint, uint8_t* pMsg, uint32_t size);

private:
	bool m_bShutDwon;
	CCenterEndpointManager m_EndpointMannager;
	uint64_t m_lastWriteTime;

	CEpoll m_ReadEpoll;
	CEpoll m_WriteEpoll;
};




#endif
