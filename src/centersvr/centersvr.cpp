#include "centersvr.h"
#include "common_sys.hpp"

#define MAX_POINT 512
#define ip_string "192.168.11.110"
#define tcp_port 9001
#define _TIME_OUT_	5
#define WRITE_INTERVAL	10

CCenterServer::CCenterServer():
m_bShutDwon(false),
m_lastWriteTime(0)
{

}

CCenterServer::~CCenterServer()
{

}


bool CCenterServer::Initalize()
{
	int maxfd = MAX_POINT+1;

	if(0 > SetRLimitForNofile(maxfd))
	{
		std::cout<<"set system limit failed!!"<<std::endl;
		return false;
	}

	if(false == m_ReadEpoll.Create(maxfd))
	{
		std::cout<<"create poll service for reading failed!"<<std::endl;
		return false;
	}

	if(false == m_WriteEpoll.Create(maxfd))
	{
		std::cout<<"create poll service for writing failed!"<<std::endl;
		return false;
	}

	if(false == m_EndpointMannager.Initialize(maxfd))
	{
		std::cout<<"initialize endpoint manager failed!"<<std::endl;
		return false;
	}

	if(false == CreateTcp())
	{
		return false;
	}

	return true;
}

void CCenterServer::SetShutDown(bool bShutDown)
{
	m_bShutDwon = bShutDown;
}

void CCenterServer::Run()
{
	while(false == m_bShutDwon)
	{
		Update();
		mSleep(1);
	}
}

bool CCenterServer::CreateTcp()
{
	int sockfd = net_create_listen_sock(ip_string, tcp_port, 1500, sock_style_tcp);
	if(sockfd < 0)
	{
		std::cout<<"create listen sock failed!!"<<std::endl;
		return false;
	}

	CCenterEndpoint* pEndpoint =  m_EndpointMannager.AllocNew();
	if(NULL == pEndpoint)
	{
		std::cout<<"create listen endpoint failed"<<std::endl;
		return false;
	}
	pEndpoint->SetSockFd(sockfd);
	if(false == m_EndpointMannager.ActiveEndPoint(pEndpoint, 0, svr_center))
	{
		std::cout<<"Active listen endpoint failed!!"<<std::endl;
		return false;
	}

	pEndpoint->SetNetAddr(net_addr_aton(ip_string, tcp_port));
	if(false == m_ReadEpoll.Add(sockfd, pEndpoint, READ_EV))
	{
		std::cout<<"Add listen endpoint into poll service failed!!"<<std::endl;
		m_EndpointMannager.ReleaseEndpoint(pEndpoint);
		return false;
	}

	return true;
}

bool CCenterServer::Update()
{
	int ifds =  m_ReadEpoll.Wait(_TIME_OUT_);
	CCenterEndpoint* pEndpoint = NULL;
	uint32_t events = 0;
	for(int i=0; i<ifds; i++)
	{
		events = m_ReadEpoll.GetEvents(i);
		pEndpoint = reinterpret_cast<CCenterEndpoint*>(m_ReadEpoll.GetDataPtr(i) );
		if(pEndpoint == m_EndpointMannager.GetEndpointBySvrId(0) )
		{
			HandleNewTcp(pEndpoint->GetSockFd() );
			continue;
		}
		while(1)
		{
			if(false == CEpoll::IsEvErr(events)&&
					true == CEpoll::IsEvRead(events)&&
					true == HandleTcpRead(pEndpoint))
			{
				break;
			}
			std::cout<<"endpoint get read error!!"<<std::endl;
			int fd = m_ReadEpoll.GetDataFd(i);
			m_ReadEpoll.Del(fd);
			if(NULL == pEndpoint || svr_none != pEndpoint->GetSvrType())
			{
				m_WriteEpoll.Del(fd);
			}

			m_EndpointMannager.ReleaseEndpoint(pEndpoint);
			break;
		}

	}

	int ActiveNum = (int)m_EndpointMannager.GetActiveNum();
	if(ActiveNum > 0 && (GameTimeIns->GetCurrMiliSec()-m_lastWriteTime) >= WRITE_INTERVAL)
	{
		int writeFds = m_WriteEpoll.Wait(_TIME_OUT_, ActiveNum);
		for(int i=0; i<writeFds; i++)
		{
			events = m_WriteEpoll.GetEvents(i);
			pEndpoint = reinterpret_cast<CCenterEndpoint*>(m_ReadEpoll.GetDataPtr(i) );
			while(1)
			{
				if(false == CEpoll::IsEvErr(events)&&
					true == CEpoll::IsEvWrite(events)&&
					true == HandleTcpWrite(pEndpoint) )
				{
					break;
				}

				std::cout<<"endpoint get write error!!"<<std::endl;
				int fd = m_ReadEpoll.GetDataFd(i);
				m_ReadEpoll.Del(fd);
				m_WriteEpoll.Del(fd);
				m_EndpointMannager.ReleaseEndpoint(pEndpoint);
				break;
			}
		}
		m_lastWriteTime = GameTimeIns->GetCurrMiliSec();
	}

	return true;
}

int CCenterServer::HandleNewTcp(int sockfd)
{
	if(sockfd <= 0)
	{
		return 0;
	}
	int num = 0;

	while(1)
	{
		net_addr_t remoteAddr;
		bzero(&remoteAddr, sizeof(net_addr_t) );
		int new_fd = net_tcp_accept(sockfd, remoteAddr);

		if(new_fd < 0)
		{
			printf("accept new connection failed!![Ret = %d, errno:%d ,errstr��%s]", new_fd, errno, strerror(errno));
			break;
		}

		if(new_fd == 0)
		{
			break;
		}

		if(num > 100)
		{
			net_close(new_fd);
			break;
		}

		net_set_nonblocking(new_fd);

		CCenterEndpoint* pEndpoint = m_EndpointMannager.AllocNew();
		if(!pEndpoint)
		{
			printf("[HandleNewTcp]end point data used out!!");
			break;
		}
		pEndpoint->SetNetAddr(remoteAddr);
		pEndpoint->SetSockFd(new_fd);
		if(false == m_ReadEpoll.Add(new_fd, pEndpoint, READ_EV) )
		{
			m_EndpointMannager.ReleaseEndpoint(pEndpoint);
			printf("epoll add failed!![fd=%d, listenfd=%d]\n", new_fd, sockfd);
			break;
		}

		printf("get new connection!![fd=%d]\n", new_fd);
		num++;
	}
	return num;
}

bool CCenterServer::HandleTcpRead(CCenterEndpoint* pEndpoint)
{
	if(NULL == pEndpoint)
	{
		return false;
	}
	int ret = pEndpoint->onReadEv();
	if(0 > ret)
	{
		std::cout<<"endpoint read event deal error!!"<<ret<<std::endl;
		return false;
	}
	static uint8_t pMsg[MSG_BUF_LEN];
	static uint32_t size;
	bzero(pMsg, sizeof(uint8_t)*MSG_BUF_LEN);
	size = 0;
	while(1)
	{
		int ret = pEndpoint->ReadMsg(pMsg, size);
		if(0 >= ret)
		{
			std::cout<<"read a msg failed!"<<ret<<std::endl;
			return false;;
		}
		if(read_msg_complete == ret)
		{
			return DealCenterMsg(pEndpoint, pMsg, size);
		}
		break;
	}

	return true;
}

bool CCenterServer::HandleTcpWrite(CCenterEndpoint* pEndpoint)
{
	if(NULL == pEndpoint)
	{
		return false;
	}

	int ret = pEndpoint->onWriteEv();
	if(0 >= ret)
	{
		std::cout<<"endpoint deal write event error!!"<<std::endl;
		return false;
	}
	//消息清空,取消写事件
	if(write_buffer_empty == ret)
	{
		DeactiveWrite(pEndpoint);
	}
	return true;
}

bool CCenterServer::ActiveWrite(CCenterEndpoint* pEndpoint)
{
	if(NULL == pEndpoint || 0 == pEndpoint->GetSockFd())
	{
		return false;
	}
	if(true == m_WriteEpoll.Add(pEndpoint->GetSockFd(), pEndpoint, WRITE_EV))
	{
		pEndpoint->ActiveWrite(true);
		return true;
	}

	return false;
}

bool CCenterServer::DeactiveWrite(CCenterEndpoint* pEndpoint)
{
	if(NULL == pEndpoint || 0 == pEndpoint->GetSockFd())
	{
		return false;
	}
	m_WriteEpoll.Del(pEndpoint->GetSockFd());
	pEndpoint->ActiveWrite(false);
	return true;
}

bool CCenterServer::SendMsgToEndpoint(CCenterEndpoint* pEndpoint, uint8_t* pMsg, uint32_t size)
{
	if(NULL == pEndpoint || 0 >= pEndpoint->GetSockFd())
	{
		return false;
	}

	if(false == pEndpoint->IsActiveWrite())
	{
		if(false == ActiveWrite(pEndpoint))
		{
			m_ReadEpoll.Del(pEndpoint->GetSockFd());
			m_WriteEpoll.Del(pEndpoint->GetSockFd());
			m_EndpointMannager.ReleaseEndpoint(pEndpoint);
		}
	}

	return pEndpoint->SendMsg(pMsg, size);
}

bool CCenterServer::DealCenterMsg(CCenterEndpoint* pEndpoint, uint8_t* pMsg, uint32_t size)
{
	if(size > MSG_SIZE+4)
	{
		return false;
	}

	stMsg* pCenterMsg = reinterpret_cast<stMsg*>(pMsg+4);
	switch(pCenterMsg->msg_head.center_head.route_type)
	{
		case route_p2p:
		{
			CCenterEndpoint* pDstEndpoint = m_EndpointMannager.GetEndpointBySvrId(pCenterMsg->msg_head.center_head.des_id);
			if(NULL != pDstEndpoint)
			{
				//目标是中心服，激活消息
				if(pDstEndpoint == m_EndpointMannager.GetEndpointBySvrId(0))
				{
					m_EndpointMannager.ActiveEndPoint(pEndpoint, pCenterMsg->msg_head.center_head.src_id, pCenterMsg->msg_head.center_head.src_type);
					std::cout<<"active endpoint"<<pEndpoint->GetSockFd()<<std::endl;
					return SendMsgToEndpoint(pEndpoint, pMsg, size);
				}
				else
				{
					return SendMsgToEndpoint(pDstEndpoint, pMsg, size);
				}
			}
			else
			{
				return false;
			}
		}
		break;
		case route_hash:
		{
			if(svr_none == pCenterMsg->msg_head.center_head.des_type)
			{
				std::list<CCenterEndpoint*>* pEndpointlist = m_EndpointMannager.GetActiveEndpointList();
				if(0 >= pEndpointlist->size())
				{
					return false;
				}
				uint32_t pos = pCenterMsg->msg_head.center_head.hash_key%pEndpointlist->size();
				static std::list<CCenterEndpoint*>::iterator listIter;
				listIter = pEndpointlist->begin();
				while(pEndpointlist->end()!=listIter && pos >0 )
				{
					listIter++;
					pos--;
				}
				CCenterEndpoint* pDstEndpoint = (*listIter);
				if(NULL == pDstEndpoint)
				{
					return false;
				}
				return SendMsgToEndpoint(pDstEndpoint, pMsg, size);
			}
			else
			{
				std::vector<CCenterEndpoint*>* pEndpointVec = m_EndpointMannager.GetEndpointBySvrType(pCenterMsg->msg_head.center_head.des_type);
				if(0 >= pEndpointVec->size())
				{
					return false;
				}
				uint32_t pos = pCenterMsg->msg_head.center_head.hash_key%pEndpointVec->size();
				CCenterEndpoint* pDstEndpoint = (*pEndpointVec)[pos];
				if(NULL == pDstEndpoint)
				{
					return false;
				}
				return SendMsgToEndpoint(pDstEndpoint, pMsg, size);
			}
		}
		break;
		case route_broadcast:
		{
			if(svr_none == pCenterMsg->msg_head.center_head.des_type)
			{
				std::list<CCenterEndpoint*>* pEndpointlist = m_EndpointMannager.GetActiveEndpointList();
				if(0 >= pEndpointlist->size())
				{
					return false;
				}
				static std::list<CCenterEndpoint*>::iterator listIter_1;
				for(listIter_1 = pEndpointlist->begin(); listIter_1 != pEndpointlist->end(); listIter_1++)
				{
					if(NULL != *(listIter_1))
					{
						SendMsgToEndpoint((*listIter_1), pMsg, size);
					}
				}
			}
			else
			{
				std::vector<CCenterEndpoint*>* pEndpointVec = m_EndpointMannager.GetEndpointBySvrType(pCenterMsg->msg_head.center_head.des_type);
				int len = (int)pEndpointVec->size();
				if(0 >= 0)
				{
					for(int i=0; i<len;i++)
					{
						if(NULL !=  (*pEndpointVec)[i])
						{
							SendMsgToEndpoint((*pEndpointVec)[i], pMsg, size);
						}
					}
				}
			}
		}
		break;
		default:
			break;
	}

	return true;
}

int main(int argc, char** argv)
{
	CCenterServer* CenterSvrIns = CCenterServer::Instance();

	if(true == CenterSvrIns->Initalize() )
	{
		CenterSvrIns->Run();
	}
    return 0;
}

