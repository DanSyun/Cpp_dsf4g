#ifndef HANDLES_H_
#define HANDLES_H_
#include <string>
#include "pb_ss_msg.pb.h"
#include "base.h"

class PBSSMsg;
class PBDBData;
class SSResponseGet;
class SSRequestGet;
class SSRequestSave;
class SSResponseSave;
struct stCenterHead;
namespace mysqlpp
{
	class StoreQueryResult;
}

class HandleBase
{
public:
	virtual bool Handle(stCenterHead& head, const PBSSMsg& msg) = 0;
	
	virtual ~HandleBase(){}
	
	//commit sql sentence to mysql server to be executed
	static bool Commit(int dbconn_id, const std::string& sql, mysqlpp::StoreQueryResult& result);
	
	//send response to one who starts the request
	static void SendResponse(stCenterHead& head, const PBSSMsg& msg);
};

class UserDataGetHandle:public HandleBase
{
public:
	bool Handle(stCenterHead& head, const PBSSMsg& msg);
	static bool DoRequest(const std::string& table_name, uint64 uid,
        const google::protobuf::RepeatedPtrField<PBDBData>& request,
        google::protobuf::RepeatedPtrField<PBDBData>& response);
	
	static void WrapResponseGet(const mysqlpp::StoreQueryResult& result,
        const google::protobuf::RepeatedPtrField<PBDBData>& request,
        google::protobuf::RepeatedPtrField<PBDBData>& response);
};

class UserDataSaveHandle:public HandleBase
{
public:
	bool Handle(stCenterHead& head, const PBSSMsg& msg);
private:
	//check version if matched for keys requested
	HandleResult CheckVersion(const std::string& table_name, const SSRequestSave& request, unsigned int uid);
	
	HandleResult DoDBVersionCheck(const std::string& table_name, const PBDBData& data_item, unsigned long long  uid_key);
	
	bool DoRequest(const std::string& table_name, const SSRequestSave& request, SSResponseSave* response, unsigned int uid);
	
	void SaveToCache(const SSRequestSave& request, unsigned int uid);
};

#include <map>
//cache for uid_key <--->version
class VersionCache:public std::map<unsigned long long, int>
{
public:
	static VersionCache& GetInstance()
	{
		static VersionCache cache;
		
		return cache;
	}
};

#endif
