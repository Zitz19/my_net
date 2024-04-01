#pragma once

#include <fstream>
#include <stdint.h>
#include <list>

#include "json.hpp"

class NetConfig
{
public:
    const std::string version_;
    const uint16_t port_;
    const std::string ip_;
    const std::string name_;

public:
    std::list<std::string> roots_;

public:
    static NetConfig ParseConfig(const std::string ConfigName);
    explicit NetConfig(const std::string version, const std::string port, const std::string ip, const std::string name);
    ~NetConfig() = default;
};