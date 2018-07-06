#include "dbconnectionmgr.h"
#include "debug.h"
#include <mysql++.h>
#include <algorithm>
#include <regex>
#include <iostream>
CDBConnectionMgr::~CDBConnectionMgr()
{
	for(auto iter = _mapConnections.begin(); iter != _mapConnections.end(); ++iter)
	{
		if(nullptr != iter->second)
		{
			iter->second->disconnect();
			
			delete iter->second;
		}
	}

	_mapConnections.clear();

	_mapConnectionUrl.clear();
}

//get one db connection by id
mysqlpp::Connection* CDBConnectionMgr::GetConnection(int id)
{
	auto iter = _mapConnections.find(id);

	if(iter == _mapConnections.end())
	{
		return nullptr;
	}

	return iter->second;
}

//create a connection to mysql server
bool CDBConnectionMgr::CreateConnection(int id, const string& url)
{
	mysqlpp::Connection *conn = new mysqlpp::Connection(false);
	
	smatch m;
	bool found = regex_search(url, m, regex("mysql://(.+):(.*)@(.+):(.+)/(.+)"));
	
	if(!found)
	{
		return false;
	}
	
	_mapConnectionUrl.insert(map<int, string>::value_type(id, url));
	
	conn->connect(m[5].str().c_str(), m[3].str().c_str(),  
				  m[1].str().c_str(), m[2].str().c_str(), 
				  stoi(m[4].str()));
	
	if(conn->connected())
	{
		_mapConnections.insert(map<int, mysqlpp::Connection*>::value_type(id, conn));
	
		std::cout<<"db connected, client version:"<<conn->client_version()<<std::endl;
		
		return true;
	}else
	{
		delete conn;
	}
	
	std::cout<<DBCONN_FAILD<<id<<std::endl;
	
	return false;
}

void CDBConnectionMgr::Ping()
{
	//1.ping connections and put failed connections into _vecOffline
	for(auto iter = _mapConnections.begin(); iter != _mapConnections.end();)
	{
		if(false == iter->second->ping())
		{
			int id = iter->first;
			
			mysqlpp::Connection *conn = iter->second;
			
			iter = _mapConnections.erase(iter);
			
			delete conn;
			
			_vecOffline.push_back(id);
		}else
		{
			++iter;
		}
	}
	
	//2.reconnect 
	for(int i = 0; i < _vecOffline.size(); )
	{
		if(true == CreateConnection(_vecOffline[i], _mapConnectionUrl[_vecOffline[i]]))
		{
			_vecOffline.erase(std::remove(_vecOffline.begin(), _vecOffline.end(),_vecOffline[i]));
		}else
		{
			++i;
		}
	}
}
