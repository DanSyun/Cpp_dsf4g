#ifndef DB_CONNECTION_MGR_H_
#define DB_CONNECTION_MGR_H_

#include <vector>
#include <map>
#include <string>
using namespace std;
namespace mysqlpp
{
	class Connection;
}
class CDBConnectionMgr
{
public:
	~CDBConnectionMgr();
public:
	//get one db connection by id
	mysqlpp::Connection* GetConnection(int id);

	//create a connection to mysql server
	bool CreateConnection(int id, const string& url);
	
	static CDBConnectionMgr* GetInstance()
	{
		static CDBConnectionMgr conn_mgr;
		
		return &conn_mgr;
	}
	//ping mysql server , if failed, try to reconnect until connected
	void Ping();
private:
	//id<--->mysqlpp::Connection
	map<int, mysqlpp::Connection*>			_mapConnections;

	map<int, string>						_mapConnectionUrl;
	
	std::vector<int>						_vecOffline;
};
#endif
