#ifndef CONFIG_H_
#define CONFIG_H_
#include "protobuf/io/zero_copy_stream_impl.h"
#include "protobuf/text_format.h"
#include "singleton.hpp"
#include <fcntl.h>

template<class PB_CONF>
class CConfig: public PB_CONF
{
public:
    typedef PB_CONF conf;
public:
	template<class PBConfig>
    bool init(const char* path, PBConfig& config)
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0) 
		{
			return false;
		}

        config.Clear();
        
		google::protobuf::io::FileInputStream input(fd);
	
        input.SetCloseOnDelete(true);
        
		if (!google::protobuf::TextFormat::Parse(&input, &config))
        {
			return false;
		}

        return true;
    }
};


#endif
