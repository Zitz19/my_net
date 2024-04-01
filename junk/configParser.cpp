#include "configParser.hpp"

using json = nlohmann::json;

NetConfig::NetConfig(const std::string version, const std::string port, const std::string ip, const std::string name)
    : version_(version)
    , port_(std::atoi(port.c_str()))
    , ip_(ip)
    , name_(name){}

NetConfig NetConfig::ParseConfig(const std::string ConfigName)
{
    std::fstream ConfigFile(ConfigName);
    json conf = json::parse(ConfigFile);

    NetConfig net_config(conf["Version"], conf["StudNet-Port"], conf["Peer-IP"], conf["Name"]);
    for (auto& root : conf["Roots"])
    {
        net_config.roots_.push_back(root);
    }

    return net_config;
}