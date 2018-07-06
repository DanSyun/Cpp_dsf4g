/*
 * endpoint.cpp
 *
 *  Created on: 2017年3月9日
 *      Author: chenhuapan
 */

#include "endpoint.h"

CBaseEndpoint::CBaseEndpoint():
	m_Sockfd(0)
{
	// TODO Auto-generated constructor stub

}

CBaseEndpoint::~CBaseEndpoint() {
	// TODO Auto-generated destructor stub
}

CBaseEndpoint::CBaseEndpoint(int sockfd):
	m_Sockfd(sockfd)
{

}

