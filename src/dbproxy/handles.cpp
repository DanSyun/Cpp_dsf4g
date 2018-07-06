#include "handles.h"
#include "dbconnectionmgr.h"
#include "pb_ss_msg.pb.h"
#include "typesdefine.h"
#include "debug.h"
#include "msg_basic.h"
#include <mysql++.h>
#include "dbdriver.h"
#include <sstream>
#include <iostream>
#include <unordered_set>

bool HandleBase::Commit(int dbconn_id, const std::string& sql, mysqlpp::StoreQueryResult& result)
{
	std::cout<<sql<<std::endl;
	
	mysqlpp::Connection* conn = CDBConnectionMgr::GetInstance()->GetConnection(dbconn_id);

	if(nullptr == conn)
	{
		return false;
	}

	mysqlpp::Query query = conn->query(sql);

	result = query.store();

	std::cout<<"query result:"<<query.errnum()<<" message: "<<query.error()<<std::endl;
	
	return (query.errnum() == 0);
}

void HandleBase::SendResponse(stCenterHead& head, const PBSSMsg& msg)
{
	MsgBasic::SendMsgToResponse(head, const_cast<PBSSMsg&>(msg));
}

bool UserDataGetHandle::Handle(stCenterHead& head, const PBSSMsg& msg)
{
	//1.make table name by concatenating "userdata_" and uid (method: uid % TOTAL_TABLE_NUM)
	const SSRequestGet& request = msg.ss_request_get();
	
	std::string table_name = USER_DATA_TABLE_PREFIX+std::to_string(request.uid() % TOTAL_TABLE_NUM);
	
	PBSSMsg get_response_msg;
	
	SSResponseGet* response = get_response_msg.mutable_ss_response_get();
	
	//2.select from tables
	bool success = DoRequest(table_name, request.uid(), request.datas(), *response->mutable_datas());
	
	HandleResult result = success ? E_HANDLE_OK : E_HANDLE_ERROR;
	
	response->set_result(result);
	
	response->set_uid(request.uid());
	
	//3.send data to one who requests
	SendResponse(head, get_response_msg);
	
	return true;
}

bool UserDataGetHandle::DoRequest(const std::string& table_name, uint64 uid,
    const google::protobuf::RepeatedPtrField<PBDBData>& request,
    google::protobuf::RepeatedPtrField<PBDBData>& response)
{
	//1.make sql sentence 
	stringstream condition;
	
	condition<<" where ";
	
	int key_size = request.size();
	
	for(int i = 0; i < key_size; ++i)
	{
		condition<<" uid_key = "<<request.Get(i).key()*KEY_MULTIPLE+uid;
		
		if((i+1) != key_size)
		{
			condition<< " or ";
		}
	}
	
	stringstream sql;
	
	sql<<"select * from "<<table_name<<condition.str();
	
	//2.execute sql sentence
	mysqlpp::StoreQueryResult result;
	if (Commit(DBConnID::E_DB_USER, sql.str(), result) == false)
        return false;

	//3.fill response data with result
	WrapResponseGet(result, request, response);
	
	return true;
}
void UserDataGetHandle::WrapResponseGet(const mysqlpp::StoreQueryResult& result,
        const google::protobuf::RepeatedPtrField<PBDBData>& request,
        google::protobuf::RepeatedPtrField<PBDBData>& response)
{
	int row_count = result.num_rows();
	
	//2.if records are found, fill data with data in records
	std::unordered_set<int> keys;
	for(int i = 0; i < row_count; ++i)
	{
		PBDBData* data_item = response.Add();
		
		int key = result[i][0] / KEY_MULTIPLE;
		
		data_item->set_key(key);
		
		data_item->set_blob(result[i][1].c_str(), result[i][1].size());
		
		data_item->set_ver(result[i][2]);

        keys.insert(key);
		std::cout<<"--key: "<<key<<" blob: "<<result[i][1]<<" version: "<<result[i][2]<<std::endl;
	}

    for (uint32 i = 0; i < request.size(); ++i)
    {
        if (keys.find(request.Get(i).key()) != keys.end())
            continue;

		PBDBData* data_item = response.Add();
		data_item->set_key(request.Get(i).key());
    }
	
}

bool UserDataSaveHandle::Handle(stCenterHead& head, const PBSSMsg& msg)
{
	//1.make table name by concatenating "userdata_" and uid (method: uid % TOTAL_TABLE_NUM)
	const SSRequestSave& request = msg.ss_request_save();
	
	std::string table_name = USER_DATA_TABLE_PREFIX+std::to_string(request.uid() % TOTAL_TABLE_NUM);
	
	//2.check version if matched for each keys, if checking of any key failed, this request fails
	PBSSMsg save_response_msg;
	
	SSResponseSave* response = save_response_msg.mutable_ss_response_save();
    HandleResult ret;
    if ((ret = CheckVersion(table_name, request, request.uid())) != E_HANDLE_OK)
	{
        if (ret == E_HANDLE_VER_DIFFER)
        {
            bool ret2 = UserDataGetHandle::DoRequest(table_name, request.uid(), request.datas(), *response->mutable_datas());
            if (!ret2) response->set_result(E_HANDLE_ERROR);
        }
		response->set_result(ret);
	}else
	//3.use mysql's replace command to update or insert records
	{
		bool success = DoRequest(table_name, request, response, request.uid());
		
		response->set_result(success ? E_HANDLE_OK : E_HANDLE_ERROR);
		
		if(success)
		{
			SaveToCache(request, request.uid());
		}
	}
	
	response->set_uid(request.uid());
	
	//4.send response to one who requests
	SendResponse(head, save_response_msg);
	
	return true;
}

//check version if matched for keys requested
HandleResult UserDataSaveHandle::CheckVersion(const std::string& table_name, const SSRequestSave& request, unsigned int uid)
{
	HandleResult ret = E_HANDLE_OK;
	
	for(int i = 0; i < request.datas_size(); ++i)
	{
		const PBDBData& data_item = request.datas(i);
		
		unsigned long long uid_key = data_item.key()*KEY_MULTIPLE + uid;
		
		//1.check version in cache 
		auto iter = VersionCache::GetInstance().find(uid_key);
		
		if(iter != VersionCache::GetInstance().end())
		{
			if((data_item.ver() - 1) != iter->second)
			{
				ret = E_HANDLE_VER_DIFFER;
				break;
			}
		}else
		//2.if cache has no member of uid_key, check version in db
		{
            if ((ret = DoDBVersionCheck(table_name, data_item, uid_key)) != E_HANDLE_OK)
			{
				break;
			}
		}
	}
	
	return ret;
}

HandleResult UserDataSaveHandle::DoDBVersionCheck(const std::string& table_name, const PBDBData& data_item, unsigned long long  uid_key)
{
	HandleResult ret = E_HANDLE_OK;
	
	stringstream sql;
		
	sql<<"select version from "<<table_name<<" where uid_key = "<<uid_key;
	
	mysqlpp::StoreQueryResult result;
	
	if(false == Commit(DBConnID::E_DB_USER, sql.str(), result))
	{
		ret = E_HANDLE_ERROR;
		
		return ret;
	}
	
	if(result.num_rows() == 0)
	{
		;
	}else
	{
		if((data_item.ver() - 1) != (int)result[0][0])
		{
			ret = E_HANDLE_VER_DIFFER;
			std::cout<<VERSION_NOT_MATCH_1<<(int)result[0][0]<<VERSION_NOT_MATCH_2<<(data_item.ver() - 1)<<std::endl;
		}
	}
	
	return ret;
}

bool UserDataSaveHandle::DoRequest(const std::string& table_name, const SSRequestSave& request, SSResponseSave* response, unsigned int uid)
{
	bool bRet = true;
	
	for(int i = 0; i < request.datas_size(); ++i)
	{
		const PBDBData& data_item = request.datas(i);
		
		stringstream sql;
		
		unsigned long long uid_key = data_item.key()*KEY_MULTIPLE + uid;
		
		sql<<"replace into "<<table_name<<" values ( "<<uid_key<<", \'";
        char buf[EN_MAX_PB_DATA_BUF];
        mysqlpp::DBDriver::escape_string_no_conn(buf, data_item.blob().c_str(), data_item.blob().size());
		sql<<buf;
        sql<<"\', "<<data_item.ver()<<" )";
		
		mysqlpp::StoreQueryResult result;
		
		if(false == Commit(DBConnID::E_DB_USER, sql.str(), result))
		{
			bRet = false;
			
			break;
		}
	}
	
	return bRet;
}

void UserDataSaveHandle::SaveToCache(const SSRequestSave& request, unsigned int uid)
{
	for(int i = 0; i < request.datas_size(); ++i)
	{
		const PBDBData& data_item = request.datas(i);
		
		unsigned long long uid_key = data_item.key()*KEY_MULTIPLE + uid;
		
		VersionCache::GetInstance()[uid_key] = data_item.ver();
	}
}



