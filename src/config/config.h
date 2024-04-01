#pragma once

#include <fstream>
#include <stdint.h>
#include <list>

#include "json.hpp"

class Config
{
public:
    const std::string version_;
    const uint16_t port_;
    const std::string ip_;
    const std::string name_;

public:
    std::list<std::string> roots_;

public:
    Config(const std::string version, const std::string port, const std::string ip, const std::string name);

    static Config ParseConfig(const std::string config_name);
};