#include "config.h"

using json = nlohmann::json;

Config::Config(const std::string version, const std::string port, const std::string ip, const std::string name)
    : version_(version)
    , port_(std::atoi(port.c_str()))
    , ip_(ip)
    , name_(name){}

Config Config::ParseConfig(const std::string config_name)
{
    std::fstream config_file(config_name);
    json conf = json::parse(config_file);

    Config net_config(conf["Version"], conf["StudNet-Port"], conf["Peer-IP"], conf["Name"]);
    for (auto& root : conf["Roots"])
    {
        net_config.roots_.push_back(root);
    }

    return net_config;
}