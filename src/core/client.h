#pragma once

#include <boost/asio.hpp>

#include "config/config.h"
#include "peer.h"

using namespace boost::asio::ip;

class Net
{
private:
    std::string hostname_;
    boost::asio::io_context io_context_;
    Peer peer_;
    std::thread main_thread_;

public:
    Net(Config config);

    void SetupHandler();
    void HandleReceive(const boost::system::error_code &error, size_t bytes_received);
    void Receive();
    void Stop();
    void Send();
};