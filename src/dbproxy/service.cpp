#include "service.h"
#include "config.h"
#include "dbconnectionmgr.h"
#include "debug.h"
#include <mysql++.h>
#include <iostream>
bool CService::Init()
{
	bool bRet = false;
	do
	{
		//1. load config
		if(false == LoadConnectionConfig())
		{
			break;
		}
		
		//2.use config to connect mysql server
		if(false == CreateDBConnections())
		{
			break;
		}
		
		_mapHandles.insert(map<PBSSMsg::MsgUnionCase, HandleBase*>::value_type(PBSSMsg::kSsRequestGet, new UserDataGetHandle()));
	
		_mapHandles.insert(map<PBSSMsg::MsgUnionCase, HandleBase*>::value_type(PBSSMsg::kSsRequestSave, new UserDataSaveHandle()));
	
		bRet = true;
	}while(0);

	return bRet;
}
	
bool CService::Uninit()
{
	for(auto iter = _mapHandles.begin(); iter != _mapHandles.end(); ++iter)
	{
		delete iter->second;
	}
	
	_mapHandles.clear();
	return true;
}

bool CService::LoadConnectionConfig()
{
	CConfig<PBDBConnectionConfig> parser;
	
	return parser.init("conninfos.cfg", _objDBConnectionInfo);
}

bool CService::CreateDBConnections()
{
	bool bRet = true;
	for(int i = 0; i < _objDBConnectionInfo.url_item_size(); ++i)
	{
		if(false  == CDBConnectionMgr::GetInstance()->CreateConnection(
					_objDBConnectionInfo.url_item(i).id(), 
					_objDBConnectionInfo.url_item(i).url()))
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}

void CService::Message(stCenterHead& head, const PBSSMsg& msg)
{
	PBSSMsg::MsgUnionCase case_num = msg.msg_union_case();
	
	auto iter = _mapHandles.find(case_num);
	
	if(iter != _mapHandles.end())
	{
		iter->second->Handle(head, msg);
	}else
	{
		std::cout<<UNKNOWN_MSG<<(int)case_num<<std::endl;
	}
}



