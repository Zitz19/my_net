#pragma once

#include <boost/asio.hpp>

#include "config/config.h"
#include "core/peer.h"

using namespace boost::asio::ip;

class App
{
private:
    Config config_;
    Peer peer_;

public:
    App(Config config);

    void Start();
    void Stop();
    void SendMessageToRoots();
    void PingRoots();
    void SearchNeighbours();
};