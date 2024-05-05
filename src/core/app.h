#pragma once

#include <boost/asio.hpp>

#include "config/config.h"
#include "core/peer.h"

using namespace boost::asio::ip;

class App
{
private:
    Config config_;
    uint32_t hostname_;
    boost::asio::io_context io_context_;
    Peer peer_;
    std::thread receiving_thread_;
    uint16_t port_;
    RouteTable route_tb_;

public:
    App(Config config);

    bool CheckRoot(boost::asio::ip::address root);
    void SetupHandler();
    void HandleReceive(const boost::system::error_code &error, size_t bytes_received);
    void Receive();
    void Stop();
    void Send();
    void SearchNeighbours();
};