#pragma once

#include "singleton.hpp"
#include "mysql++.h"
#include "base.h"

class DBManager: public Singleton<DBManager>
{
public:
    bool Init();
    bool Query(const char* sql);
    void OnMessage(stTcpHead& head, PBCSMsg& cs_msg);
    void OnAuthorization(const PBCSMsg& cs_msg, PBCSMsg& msg);
private:
    mysqlpp::Connection         _conn;
    mysqlpp::StoreQueryResult   _result;
    char    _sql[512];
    uint64  _uid_index;
};

