/*
 * center_endpoint.cpp
 *
 *  Created on: 2017年3月9日
 *      Author: Administrator
 */

#include "center_endpoint.h"
#include <iostream>

CCenterEndpoint::CCenterEndpoint():
	m_Id(0),
	m_Type(svr_none),
	m_Hnd(0),
	m_activeTime(0),
	m_writeActive(false)
{
	// TODO Auto-generated constructor stub
	m_RevTmp.SetMaxSize(MSG_BUF_LEN*2);
	m_RevTmp.SetReallocSize(MSG_BUF_LEN*2);
	m_RevBuffer.SetMaxSize(MSG_BUF_LEN*100);
	m_RevBuffer.SetReallocSize(MSG_BUF_LEN*2);
	m_SendTmp.SetMaxSize(MSG_BUF_LEN*2);
	m_SendTmp.SetReallocSize(MSG_BUF_LEN*2);
	m_SendBuffer.SetMaxSize(MSG_BUF_LEN*100);
	m_SendBuffer.SetReallocSize(MSG_BUF_LEN*2);
}

CCenterEndpoint::CCenterEndpoint(int sock):
	CBaseEndpoint(sock),
	m_Id(0),
	m_Type(svr_none),
	m_Hnd(0),
	m_activeTime(0),
	m_writeActive(false)
{
	m_RevTmp.SetMaxSize(MSG_BUF_LEN*2);
	m_RevTmp.SetReallocSize(MSG_BUF_LEN*2);
	m_RevBuffer.SetMaxSize(MSG_BUF_LEN*100);
	m_RevBuffer.SetReallocSize(MSG_BUF_LEN*2);
	m_SendTmp.SetMaxSize(MSG_BUF_LEN*2);
	m_SendTmp.SetReallocSize(MSG_BUF_LEN*2);
	m_SendBuffer.SetMaxSize(MSG_BUF_LEN*100);
	m_SendBuffer.SetReallocSize(MSG_BUF_LEN*2);
}

CCenterEndpoint::~CCenterEndpoint() {
	// TODO Auto-generated destructor stub
}

void CCenterEndpoint::Init(int iHnd)
{
	if(0 == m_Hnd)m_Hnd = iHnd;
}

void CCenterEndpoint::Reset()
{
	m_RevTmp.Reset();
	m_RevBuffer.Reset();
	m_SendTmp.Reset();
	m_SendBuffer.Reset();
	m_Id = 0;
	m_Type = svr_none;
	m_activeTime = 0;
	if(m_Sockfd > 0)
	{
		net_close(m_Sockfd);
	}
}

int CCenterEndpoint::onReadEv()
{
	if(false == m_RevTmp.CanWrite(MSG_BUF_LEN))
	{
		return read_buffer_lacking;
	}

	int iRec = net_tcp_recv(m_Sockfd, m_RevTmp.WriteBuf(), MSG_BUF_LEN);
	if(iRec <= 0)
	{
		return networking_error;
	}

	if(false == m_RevTmp.WriteSlip(iRec))
	{
		return read_buffer_lacking;
	}

	//前4个字节表示长度
	while(m_RevTmp.CanRead(4))
	{
		static uint8_t tempLen[4];
		bcopy(m_RevTmp.ReadBuf(), tempLen, 4);
		int length = *(reinterpret_cast<int*>(tempLen));
		if((4+MSG_SIZE) > length || length > MSG_BUF_LEN)
		{
			m_RevTmp.Reset();
			return message_error;
		}
		if(false == m_RevTmp.CanRead(length) )
		{
			break;
		}
		if(false == m_RevBuffer.CanWrite(length))
		{
			m_RevTmp.Reset();
			return read_buffer_lacking;
		}
		bcopy(m_RevTmp.ReadBuf(), m_RevBuffer.WriteBuf(length), length);
		m_RevTmp.Reset();
		break;
	}

	return error_none;
}

int CCenterEndpoint::onWriteEv()
{
	int length =  m_SendBuffer.ReadSize();
	if(0 < length)
	{
		int iRec = net_tcp_send(m_Sockfd, m_SendBuffer.ReadBuf(), length);
		if(0 >= iRec)
		{
			return networking_error;
		}
		iRec = (iRec > length) ? length : iRec;
		m_SendBuffer.ReadSlip(iRec);
		if(0 >= m_SendBuffer.ReadSize())
		{
			return write_buffer_empty;
		}
	}

	return error_none;
}

int CCenterEndpoint::ReadMsg(uint8_t* pMsg, uint32_t& size)
{
	//前4个字节表示消息长度
	if(false == m_RevBuffer.CanRead(4))
	{
		return false;
	}

	static uint8_t tempLen[4];
	bcopy(m_RevBuffer.ReadBuf(), tempLen, 4);
	int length = *(reinterpret_cast<int*>(tempLen));
	if(length < 4 || length > MSG_BUF_LEN)
	{
		std::cout<<"read msg error"<<std::endl;
		m_RevBuffer.Reset();
		return message_error;
	}
	if(false == m_RevBuffer.CanRead(length))
	{
		return message_error;
	}

	bcopy(m_RevBuffer.ReadBuf(length), pMsg, length);
	size = length;
	return read_msg_complete;
}

bool CCenterEndpoint::SendMsg(uint8_t* pMsg, uint32_t size)
{
	return m_SendBuffer.WriteBuf(pMsg, size);
}


////////////////////////////////////////////////////////////////////////////////////
/// CCenterEndpointManager
/// ///////////////////////////////////////////////////////////////////////////////

CCenterEndpointManager::CCenterEndpointManager():
	m_MaxPoint(0),
	m_EndpointPool(NULL)
{
	m_mapEndpointType.clear();
	m_mapEndpointServerID.clear();
	vecFree.clear();
	listActive.clear();
}

CCenterEndpointManager::~CCenterEndpointManager()
{
	delete[] m_EndpointPool;
}

bool CCenterEndpointManager::Initialize(uint32_t max)
{
	if(0 >= max)
	{
		return false;
	}

	m_EndpointPool = new CCenterEndpoint[max];
	for(int i=0; i<max;i++)
	{
		m_EndpointPool[i].Init(i);
		vecFree.push_back(&m_EndpointPool[i]);
	}
	m_MaxPoint = max;
	return true;
}

void CCenterEndpointManager::ReIndex()
{
	if(NULL == m_EndpointPool)
	{
		return;
	}
	for(int i=0 ; i<m_MaxPoint; i++)
	{
		static CCenterEndpoint* pEndpoint= &m_EndpointPool[i];
		if(svr_none != pEndpoint->GetSvrType())
		{
			m_mapEndpointType[pEndpoint->GetSvrType()].push_back(pEndpoint);
		}

		if(0 != pEndpoint->GetSvrId())
		{
			m_mapEndpointServerID[pEndpoint->GetSvrId()] = pEndpoint;
		}
	}
}

CCenterEndpoint* CCenterEndpointManager::AllocNew()
{
	if(0 < vecFree.size())
	{
		CCenterEndpoint* point = vecFree.back();
		vecFree.pop_back();
		return point;
	}

	return NULL;
}

bool CCenterEndpointManager::ActiveEndPoint(CCenterEndpoint* point, int svrId, int svrType)
{
	uint32_t pos = point->GetHnd();
	if(pos >= m_MaxPoint)
	{
		return false;
	}

	CCenterEndpoint* pEndpoint = &m_EndpointPool[pos];
	if(svr_none != point->GetSvrType() || pEndpoint != point)
	{
		return false;
	}

	pEndpoint->SetSvrId(svrId);
	pEndpoint->SetSvrType(svrType);

	m_mapEndpointServerID[svrId] = pEndpoint;
	m_mapEndpointType[svrType].push_back(pEndpoint);
	listActive.push_back(point);

	return true;
}

CCenterEndpoint* CCenterEndpointManager::GetEndpointByPos(uint32_t pos)
{
	if(pos > m_MaxPoint)
	{
		return NULL;
	}

	return &m_EndpointPool[pos];
}

void CCenterEndpointManager::ReleaseEndpointByPos(uint32_t pos)
{
	if(pos >=  m_MaxPoint)
	{
		return;
	}

	CCenterEndpoint* pEndpoint = &m_EndpointPool[pos];

	ReleaseEndpoint(pEndpoint);
}

CCenterEndpoint* CCenterEndpointManager::GetEndpointBySvrId(uint32_t svrId)
{
	if(m_mapEndpointServerID.end() != m_mapEndpointServerID.find(svrId))
	{
		return m_mapEndpointServerID[svrId];
	}

	return NULL;
}

void CCenterEndpointManager::ReleaseEndpointBySvrId(uint32_t svrId)
{
	if(m_mapEndpointServerID.end() != m_mapEndpointServerID.find(svrId))
	{
		ReleaseEndpoint(m_mapEndpointServerID[svrId]);
	}
}

void CCenterEndpointManager::ReleaseEndpoint(CCenterEndpoint* pEndpoint)
{
	if(NULL == pEndpoint)
	{
		return;
	}
	mapEndpointIter = m_mapEndpointServerID.find(pEndpoint->GetSvrId());
	if(m_mapEndpointServerID.end() != mapEndpointIter)
	{
		m_mapEndpointServerID.erase(mapEndpointIter);
	}
	mapEndpintTypeIter = m_mapEndpointType.find(pEndpoint->GetSvrType());
	if(m_mapEndpointType.end() != mapEndpintTypeIter)
	{
		for(vecEndpointIter=mapEndpintTypeIter->second.begin(); vecEndpointIter != mapEndpintTypeIter->second.end();vecEndpointIter++)
		{
			if(pEndpoint == *(vecEndpointIter))
			{
				mapEndpintTypeIter->second.erase(vecEndpointIter);
				break;
			}
		}
	}
	if(svr_none != pEndpoint->GetSvrType())
	{
		listActive.remove(pEndpoint);
	}
	pEndpoint->Reset();
	vecFree.push_back(pEndpoint);
}

std::vector<CCenterEndpoint*>* CCenterEndpointManager::GetEndpointBySvrType(int type)
{
	if(m_mapEndpointType.end() != m_mapEndpointType.find(type))
	{
		return &m_mapEndpointType[type];
	}

	return NULL;
}

