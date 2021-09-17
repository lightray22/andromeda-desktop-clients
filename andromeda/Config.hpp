#ifndef LIBA2_CONFIG_H_
#define LIBA2_CONFIG_H_

#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;

class Config
{
public:
    Config();
    
    static constexpr int API_VERSION = 2;

    class Exception : public Utilities::Exception { public:
        Exception(const std::string& message) :
            Utilities::Exception("Config Error: "+message){}; };

    class APIVersionException : public Exception { public:
        APIVersionException(int version) : 
            Exception("API Version is "+std::to_string(version)+
                    ", require "+std::to_string(API_VERSION)){}; };

    class AppMissingException : public Exception { public:
        AppMissingException(const std::string& appname) :
            Exception("Missing app: "+appname){}; };

    void Initialize(Backend& backend);

    bool isReadOnly() { return this->readOnly; }

    unsigned int getUploadMaxBytes() { return this->uploadMaxBytes; }
    unsigned int getUploadMaxFiles() { return this->uploadMaxFiles; }

private:
    Debug debug;

    bool readOnly;

    unsigned int uploadMaxBytes;
    unsigned int uploadMaxFiles;
};

#endif