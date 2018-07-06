/*
@Filename:typesdefine.h
@Author:yuanzuochao
@Date:2017-03-08
@Description:
	This file defines struct or enum types to be used in db_mysql project
**/

#ifndef _TYPES_DEFINE_H_
#define _TYPES_DEFINE_H_
#include <string>
using namespace std;

#define DO 	do{

#define END 	}while(0);

#define TOTAL_TABLE_NUM		(512)

#define KEY_MULTIPLE		((uint64)10000000000)

#define USER_DATA_TABLE_PREFIX "userdata_"

namespace DBConnID
{
	enum ConnectionID
	{
		E_DB_USER = 1,
		E_DB_GAME = 2	
	};
}

//describe connection info.
struct stConnectionInfo
{
	//db url
	string 			url;

	//backup db url
	string			backup_url;

	//connection id
	DBConnID::ConnectionID 	dbconn_id;
};
//wrap node -->check node
struct stW2CPackageInfo
{
	//write or read flag
	//true for write, false for read
	bool 		 	write;
	
	//connnection id(obtained by which command)
	DBConnID::ConnectionID  	dbconn_id;
};

//check node-->commit node
struct stC2CPackageInfo
{
	//write or read flag
	//true for write, false for read
	bool 		 	write;
	
	//sql sentence that would be executed
	string  		sql;
	
	//connnection id(obtained by which command)
	DBConnID::ConnectionID  	dbconn_id;
};
#endif
