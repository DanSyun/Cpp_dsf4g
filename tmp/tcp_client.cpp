/*
 * tcp_client.cpp
 *
 *  Created on: 2017年3月7日
 *      Author: chenhuapan
 */

#include "tcp_client.h"
#include "net_util.hpp"

CTcpClient::CTcpClient():m_iSock(0) {
	// TODO Auto-generated constructor stub

}

CTcpClient::~CTcpClient() {
	// TODO Auto-generated destructor stub
	Close();
}

void CTcpClient::Close()
{
	if(m_iSock > 0)
	{
		net_close(m_iSock);
		m_iSock = 0;
	}
}

int CTcpClient::Connect(const char* pszIpStr, uint32_t port, bool bNoBloclkFlag)
{
	if(m_iSock != 0)
	{
		Close();
	}
	m_iSock = net_tcp_sock();
	if(m_iSock <= 0)
	{
		return -1;
	}

	if(net_connect(m_iSock, pszIpStr, port) != 0)
	{
		Close();
		return -2;
	}

	if(bNoBloclkFlag)
	{
		net_set_nonblocking(m_iSock);
	}

	return m_iSock;
}

int CTcpClient::Send(char* pBuf, int iLen)
{
	return net_tcp_send(m_iSock, pBuf, iLen);
}

int CTcpClient::Recv(char* pBuf, int iLen)
{
	return net_tcp_recv(m_iSock, pBuf, iLen);
}
