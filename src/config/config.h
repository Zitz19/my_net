#pragma once

#include <fstream>
#include <stdint.h>
#include <list>

#include "json.hpp"

class Config
{
public:
    const std::string version_;
    std::string ip_;
    const uint16_t port_;
    std::string name_;
    const uint32_t pid_;

public:
    std::list<std::string> roots_;

public:
    Config(const std::string version, const uint16_t port, const uint32_t pid);

    static Config ParseConfig(const std::string config_name);
};