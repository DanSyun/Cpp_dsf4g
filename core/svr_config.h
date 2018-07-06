#pragma once

#include <fcntl.h>

#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "singleton.hpp"
#include "pb_svr_config.pb.h"

namespace dan {

class SvrConfig:
    public PBLogicConfig,
    public dan::Singleton<SvrConfig>
{
private:
    typedef PBLogicConfig base;
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

        base::CopyFrom(config);
        return true;
    }
};

}

