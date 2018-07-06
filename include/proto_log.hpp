#pragma once

#include <string>
#include <iostream>

#include "singleton.hpp"
#include "protobuf/message.h"
#include "protobuf/text_format.h"
#include "protobuf/stubs/common.h"

inline static void NewProtoLogHandler(google::protobuf::LogLevel level, const char* filename, int line, const std::string& message);

inline static void TraceProto(const google::protobuf::Message& message, uint32 msg_id = -1, const char* info = "")
{
    std::string str;
    google::protobuf::TextFormat::PrintToString(message, &str);
    std::cout << "------"
                << "---time=" << GetTime()
                << "---msg_name=" << message.GetTypeName()
                << "---msg_id=" << msg_id
                << "---info=" << info
                << "------" << std::endl;
    std::cout << str ;
}

class ProtoLog: public Singleton<ProtoLog>
{
public:
    void Init()
    {
        google::protobuf::SetLogHandler(NewProtoLogHandler);
    }
    void LogHandle(google::protobuf::LogLevel level, char const * filename, int line, std::string const & message)
    {
        err = message;
    }
    std::string err;
};

static void NewProtoLogHandler(google::protobuf::LogLevel level, const char* filename, int line, const std::string& message)
{
    ProtoLog::Instance()->LogHandle(level, filename, line, message);
/*    std::cout << "[protobuf err]\n"
        << "level: " << level << "\n"
        << "file: " << filename << "\n" 
        << "line: " << line << "\n"
        << "err: " << message << "\n"
        << "[protobuf err]" << std::endl;*/
}

