#ifndef CSERVER_H_
#define CSERVER_H_
#include "pb_svr_config.pb.h"
#include "pb_ss_msg.pb.h"
#include "handles.h"
#include <map>
class ssmsg;
struct stCenterHead;
using namespace std;
class CService
{
public:
	bool Init();
	
	bool Uninit();
	
	void Message(stCenterHead& head, const PBSSMsg& msg);
public:
	static CService& Instance()
	{
		static CService service;
		
		return service;
	}
private:
	bool LoadConnectionConfig();
	
	bool CreateDBConnections();
private:

	PBDBConnectionConfig					_objDBConnectionInfo;
	
	map<PBSSMsg::MsgUnionCase, HandleBase*>	_mapHandles;
};

#endif 
