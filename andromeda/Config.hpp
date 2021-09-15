#ifndef LIBA2_CONFIG_H_
#define LIBA2_CONFIG_H_

#include <nlohmann/json_fwd.hpp>

class Config
{
public:
    void Initialize(const nlohmann::json& data);
};

#endif