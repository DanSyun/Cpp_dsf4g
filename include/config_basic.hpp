#pragma once

#include <fcntl.h>

#include "protobuf/io/zero_copy_stream_impl.h"
#include "protobuf/text_format.h"
#include "singleton.hpp"
#include "pb_config.pb.h"


template<class CONF_TYPE>
class ProtoConfigBasic: public CONF_TYPE
{
protected:
    typedef CONF_TYPE _config;
protected:
    bool Init(const char* path, PBConfig &config)
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0) return false;

        google::protobuf::io::FileInputStream input(fd);
        input.SetCloseOnDelete(true);
        if (!google::protobuf::TextFormat::Parse(&input, &config))
            return false;

        return true;
    }
};

class FruitConfig:
    public Singleton<FruitConfig>,
    public ProtoConfigBasic<PBFruitConfig>
{
private:
    typedef ProtoConfigBasic<PBFruitConfig> base;
public:
    bool Init(const char* path)
    {
        PBConfig config;
        if (!base::Init(path, config))
            return false;

        _config::CopyFrom(config.fruit());
        return true;
    }
};


