#include "cipher.hpp"
#include "msg.h"

bool Msg::PlayerLogin(Session* psession)
{
    Player* pplayer = NULL;
    uint64 uid = psession->GetCSMsg().uid();
    const std::string& strtoken = psession->GetCSMsg().cs_request_login().token();
    // ��֤token
    int len;
    uint8 sztoken[EN_MAX_RSA_TOKEN_LEN] = {0};
    if (RSACipher::Instance()->DecryptWithPubKey((uint8*)strtoken.c_str(), strtoken.size(), sztoken, len) == false)
        return false;

    stAuthToken token;
    memcpy(&token, sztoken, sizeof(token));
    if (token.uid != uid)
        return false;
    if (token.time + EN_TOKEN_EXPIRE_SEC <= GetTime())
        return false;

    // �Ƿ��ظ���¼
    pplayer = PlayerManager::Instance()->GetPlayer(uid);
    if (pplayer != NULL)
    {
        // �����ͬһ���������Ӻ���ĵ�¼��
        if (pplayer->GetTcpHead().socket_pos == psession->GetTcpHead().socket_pos &&
            pplayer->GetTcpHead().create_time == psession->GetTcpHead().create_time)
            return true;
        // ����ͬһ�������ߵ���һ����
        else
            Msg::PlayerLogout(pplayer);
    }

    // tcpsvr ��������
    if (_pserver->NotifyTcpLogicIndex(psession->GetTcpHead(), uid) == false)
        return false;

    pplayer = PlayerManager::Instance()->CreatePlayer(uid);
    if (pplayer == NULL)
        return false;

    pplayer->SetTcpHead(psession->GetTcpHead());
    return true;
}

void Msg::PlayerLogout(uint64 uid, bool notify_cli)
{
    Player* pplayer = PlayerManager::Instance()->GetPlayer(uid);
    if (pplayer == NULL) return;

    PlayerLogout(pplayer, notify_cli);
}

void Msg::PlayerLogout(Player* pplayer, bool notify_cli)
{
    if (pplayer == NULL) return;

    // �޸�db
    PBInterMsg inter;
    inter.mutable_inter_logout()->set_uid(pplayer->uid());

    // �߳���Ϣ
    if (notify_cli == true)
    {
        PBCSMsg cs;
        cs.mutable_cs_response_logout();
        SendMsgToClient(pplayer->GetTcpHead(), cs, true);
    }

    // ɾ������
    PlayerManager::Instance()->ReleasePlayer(pplayer->uid());
}

