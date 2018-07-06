#pragma once

#include "singleton.hpp"
#include "proto_config.hpp"
#include "pb_config_svr.pb.h"

namespace dan {

class TcpConfig:
    public dan::Singleton<TcpConfig>,
    public ProtoConfigBasic<PBTcpConfig, PBSvrConfig>
{
private:
    typedef ProtoConfigBasic<PBTcpConfig, PBSvrConfig> base;
public:
    bool Init(const char* path)
    {
        PBSvrConfig config;
        if (!base::Init(path, config))
            return false;

        _config::CopyFrom(config.tcp());
        return true;
    }
};

}

