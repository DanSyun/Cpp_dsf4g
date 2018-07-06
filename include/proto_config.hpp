#pragma once

#include <fcntl.h>

#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "singleton.hpp"
#include "pb_config.pb.h"

namespace dan {

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

class TestConfig:
    public dan::Singleton<TestConfig>,
    public ProtoConfigBasic<PBTestConfig>
{
private:
    typedef ProtoConfigBasic<PBTestConfig> base;
public:
    bool Init(const char* path)
    {
        PBConfig config;
        if (!base::Init(path, config))
            return false;

        _config::CopyFrom(config.test());
        return true;
    }
};

}

