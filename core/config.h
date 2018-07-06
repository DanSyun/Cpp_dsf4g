#pragma once

#include "pb_svr_config.pb.h"
#include "protobuf/io/zero_copy_stream_impl.h"
#include "protobuf/text_format.h"

#include <unordered_map>

// logic≤ø ≈‰÷√
class SvrConfig: public PBLogicConfig
{
public:
    bool Init(const char* path)
    {
        PBLogicConfig config;
        int fd = open(path, O_RDONLY);
        if (fd < 0) return false;

        google::protobuf::io::FileInputStream input(fd);
        input.SetCloseOnDelete(true);
        if (!google::protobuf::TextFormat::Parse(&input, &config))
            return false;

        PBLogicConfig::CopyFrom(config);
        return true;
    }
};

// tcp≤ø ≈‰÷√
class TcpConfig: public PBTcpConfig
{
public:
    bool Init(const char* path)
    {
        PBTcpConfig config;
        int fd = open(path, O_RDONLY);
        if (fd < 0) return false;

        google::protobuf::io::FileInputStream input(fd);
        input.SetCloseOnDelete(true);
        if (!google::protobuf::TextFormat::Parse(&input, &config))
            return false;

        PBTcpConfig::CopyFrom(config);
        return true;
    }
};

// center≤ø ≈‰÷√
class CenterConfig: public PBCenterConfig
{
public:
    typedef PBCenterConfig  base;
    bool Init(const char* path)
    {
        PBCenterConfig config;
        int fd = open(path, O_RDONLY);
        if (fd < 0) return false;

        google::protobuf::io::FileInputStream input(fd);
        input.SetCloseOnDelete(true);
        if (!google::protobuf::TextFormat::Parse(&input, &config))
            return false;

        PBCenterConfig::CopyFrom(config);

        _hash_index.clear();
        for (uint32 i = 0; i < base::svrs_size(); ++i)
        {
            _hash_index[base::svrs(i).svr_type()] = i;
        }
        return true;
    }
    const PBLogicSvr* GetSvr(uint32 svr_type)
    {
        if (_hash_index.find(svr_type) == _hash_index.end())
            return NULL;
        else
            return &base::svrs(_hash_index[svr_type]);
    }
private:
    std::unordered_map<uint32, uint32> _hash_index;
};

