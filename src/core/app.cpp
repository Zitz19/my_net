#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "app.h"

App::App(Config config)
    : config_(config)
    , peer_(address_v4::any(), config.port_)
{
    peer_.SetPID(config.pid_);
    std::list<address> roots;
    for (std::string addr : config.roots_)
    {
        roots.push_back(make_address(addr));
    }
    peer_.SetRoots(roots);
}

void App::Start()
{
    peer_.StartReceive();
}

void App::Stop()
{
    peer_.StopReceive();
}

void App::SendMessageToRoots()
{
    bool is_end = false;
    std::string message = "<connected>";
    peer_.SendToRoots(message);
    while (std::getline(std::cin, message))
    {
        if (message == "-exit")
        {
            message = "<disconnected>";
            is_end = true;
        }
        peer_.SendToRoots(message);
        if (is_end)
        {
            return;
        }
        std::cout << "You: " << message << '\n';
    }
}

void App::PingRoots()
{
    peer_.PingRoots();
}

void App::SearchNeighbours()
{
    peer_.SearchNeighbours();
}