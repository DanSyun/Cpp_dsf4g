#include "db_mng.h"
#include "msg_basic.h"
#include "hash.hpp"
#include "cipher.hpp"

bool DBManager::Init()
{
    snprintf(_sql, sizeof(_sql), "select * from uid_index");
    Query(_sql);

    if (_result.num_rows() != 1)
        return false;

    _uid_index = _result[0][0];

    return true;
}

bool DBManager::Query(const char* sql)
{
    const PBLogicConfig& conf = MsgBasic::GetConf();
    if (_conn.connected() == false)
        if (_conn.connect(conf.db_name().c_str(), conf.db_ip().c_str(), conf.db_account().c_str(),
            conf.db_password().c_str(), conf.db_port()) == false)
            return false;

	mysqlpp::Query query = _conn.query(_sql);
	_result = query.store();

    if (query.errnum() != 0)
        return false;

    return true;
}

void DBManager::OnMessage(stTcpHead & head, PBCSMsg& cs_msg)
{
    PBCSMsg response;
    switch (cs_msg.msg_union_case())
    {
        case PBCSMsg::kCsRequestAuth:
            OnAuthorization(cs_msg, response);
            break;
        default: break;
    }
    MsgBasic::SendMsgToClient(head, response, true);
}

void DBManager::OnAuthorization(const PBCSMsg& cs_msg, PBCSMsg& msg)
{
    CSResponseAuth& response = *msg.mutable_cs_response_auth();
    const CSRequestAuth& request = cs_msg.cs_request_auth();
    uint32 index = GetHash(request.account().c_str(), request.account().size())% 512;

    snprintf(_sql, sizeof(_sql), "select uid, password from account_%u where account_type = %u and account_name = '%s'",
        index, request.type(), request.account().c_str());
    if (Query(_sql) == false)
    {
        response.set_result(EN_MESSAGE_DB_ERR);
        return;
    }

    uint64 uid;
    if (_result.num_rows() != 0)
    {
        // 密码
        if (request.type() != EN_ACC_GEUST)
        {

        }
        uid = _result[0][0];
    }
    else
    {
        // 匿名账号自动注册
        if (request.type() == EN_ACC_GEUST)
        {
            snprintf(_sql, sizeof(_sql), "update uid_index set uid = %llu where uid = %llu",
                _uid_index + 1, _uid_index);
            if (Query(_sql) == false)
            {
                response.set_result(EN_MESSAGE_DB_ERR);
                return;
            }
            _uid_index++;
            snprintf(_sql, sizeof(_sql), "insert into account_%u (account_type, account_name, uid) values (%u, '%s', %llu)",
                index, request.type(), request.account().c_str(), _uid_index);
            if (Query(_sql) == false)
            {
                response.set_result(EN_MESSAGE_DB_ERR);
                return;
            }
            uid = _uid_index;
        }
        else
        {
            response.set_result(EN_MESSAGE_ACCOUNT_NOT_FOUND);
            return;
        }
    }
    // 生成token，逻辑登录时验证
    stAuthToken token;
    token.uid = uid;
    token.time = GetTime();
    int len;
    uint8 sztoken[EN_MAX_RSA_TOKEN_LEN] = {0};
    if (RSACipher::Instance()->EncryptWithPriKey((uint8*)&token, sizeof(token), sztoken, len) == false)
    {
        response.set_result(EN_MESSAGE_SYS_ERR);
        return;
    }

    response.set_result(EN_MESSAGE_OK);
    response.set_uid(uid);
    response.set_token(sztoken, len);
}

