/*
 * tcp_client.h
 *
 *  Created on: 2017年3月7日
 *      Author: chenhuapan
 */

#ifndef CORE_TCP_CLIENT_H_
#define CORE_TCP_CLIENT_H_

#include "common_include.h"

class CTcpClient {
public:
	CTcpClient();
	virtual ~CTcpClient();

	int Connect(const char* pszIpStr, uint32_t port, bool bNoBlockFlag );
	int GetSockFd(){return m_iSock;}

	void Close();

	int Send(char* pBuf, int iLen);
	int Recv(char* pBuf, int iLen);

private:
	int	m_iSock;
};

#endif /* CORE_TCP_CLIENT_H_ */
