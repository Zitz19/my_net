#include "config.h"

using json = nlohmann::json;

Config::Config(const std::string version, const std::string port, uint32_t pid)
    : version_(version)
    , port_(std::atoi(port.c_str()))
    , pid_(pid) {}

Config Config::ParseConfig(const std::string config_name)
{
    std::fstream config_file(config_name);
    json conf = json::parse(config_file);

    Config app_config(conf["Version"], conf["StudNet-Port"], conf["PID"]);
    if (conf.contains("Peer-IP"))
    {
        app_config.ip_ = conf["Peer-IP"];
    }
    if (conf.contains("Hostname"))
    {
        app_config.name_ = conf["Hostname"];
    }
    if (conf.contains("Roots") && conf["Roots"].size() > 0)
    {
        for (auto& root : conf["Roots"])
        {
            app_config.roots_.push_back(root);
        }
    }

    return app_config;
}