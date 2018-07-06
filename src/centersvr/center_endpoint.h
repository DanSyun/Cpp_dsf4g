/*
 * center_endpoint.h
 *
 *  Created on: 2017年3月9日
 *      Author: Administrator
 */

#ifndef SRC_CENTERSVR_CENTER_ENDPOINT_H_
#define SRC_CENTERSVR_CENTER_ENDPOINT_H_

#include "common_include.h"
#include "center_buffer.h"
#include "base.h"
#include "endpoint.h"
#include <map>
#include <vector>
#include <list>

#define _CENTER_READ_BUFFER_LEN_ (MSG_BUF_LEN*10)
#define _CENTER_WRITE_BUFFER_LEN_ (MSG_BUF_LEN*100)

enum center_endpoint_ret{
	read_msg_complete = 2,
	write_buffer_empty = 1,
	error_none = 0,
	read_buffer_lacking = -1,
	write_buffer_lacking = -2,
	networking_error = -3,
	message_error= -4,
};

class CCenterEndpoint: public CBaseEndpoint
{
public:
	CCenterEndpoint();
	CCenterEndpoint(int sock);
	virtual ~CCenterEndpoint();
	void Init(int iHnd);
	void Reset();

public:
	int onReadEv();
	int onWriteEv();
	int GetSvrId(){return m_Id;};
	void SetSvrId(int svrId){m_Id = svrId;}
	int GetSvrType(){return m_Type;};
	void SetSvrType(int svrType){m_Type = svrType;};
	uint64_t GetActiveTime(){return m_activeTime;};
	void SetActiveTime(uint64_t timeSec){m_activeTime=timeSec;};
	int GetHnd(){return m_Hnd;};
	void ActiveWrite(bool bwrite){m_writeActive = bwrite;};
	bool IsActiveWrite(){return m_writeActive;};

	bool SendMsg(uint8_t* pMsg, uint32_t size);
	int ReadMsg(uint8_t* pMsg, uint32_t& size);
private:
	int m_Id;
	int m_Type;
	int m_Hnd;
	uint64_t m_activeTime;
	bool m_writeActive;
private:
	//接收队列
	CByteBuffer m_RevTmp;
	CByteBuffer m_RevBuffer;
	//发送队列
	CByteBuffer m_SendTmp;
	CByteBuffer m_SendBuffer;

};

class CCenterEndpointManager
{
public:
	CCenterEndpointManager();
	virtual ~CCenterEndpointManager();

	bool Initialize(uint32_t max);
	void ReIndex();

	CCenterEndpoint* AllocNew();
	bool ActiveEndPoint(CCenterEndpoint* point, int svrId, int svrType);

	CCenterEndpoint* GetEndpointByPos(uint32_t pos);
	void ReleaseEndpointByPos(uint32_t pos);

	CCenterEndpoint* GetEndpointBySvrId(uint32_t svrId);
	void ReleaseEndpointBySvrId(uint32_t svrId);

	void ReleaseEndpoint(CCenterEndpoint* pEndpoint);

	//即取即用
	std::vector<CCenterEndpoint*>* GetEndpointBySvrType(int type);
	//获取激活
	uint32_t GetActiveNum(){return (uint32_t)listActive.size();}
	std::list<CCenterEndpoint*>* GetActiveEndpointList(){return &listActive;};

private:
	uint32_t m_MaxPoint;
	CCenterEndpoint* m_EndpointPool;
	//未使用
	std::vector<CCenterEndpoint*> vecFree;
	std::vector<CCenterEndpoint*>::iterator vecEndpointIter;
	//激活
	std::list<CCenterEndpoint*> listActive;
	//索引
	std::map<int, std::vector<CCenterEndpoint*> > m_mapEndpointType;
	std::map<int, std::vector<CCenterEndpoint*> >::iterator mapEndpintTypeIter;
	std::map<int, CCenterEndpoint*> m_mapEndpointServerID;
	std::map<int, CCenterEndpoint*>::iterator mapEndpointIter;

};


#endif /* SRC_CENTERSVR_CENTER_ENDPOINT_H_ */
