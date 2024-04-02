#pragma once

#include <boost/asio.hpp>

#include "config/config.h"
#include "peer.h"

class Net
{
private:
    std::string hostname_;
    Peer unicast_peer_;
    MultiPeer root_peer_;
    std::thread thread_;

public:
    Net(Config config);

    void SetupHandler();
    void HandleReceive(const boost::system::error_code &error, size_t bytes_received);
    void Receive();
    void Send();
};