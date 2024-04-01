#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "client.h"

Net::Net(Config config)
    : hostname_(config.name_)
    , unicast_peer_(boost::asio::ip::address_v4::any(), config.port_)
    , root_peer_(boost::asio::ip::make_address(config.ip_), config.port_)
{
    std::list<boost::asio::ip::address> roots;
    for (std::string addr : config.roots_)
    {
        roots.push_back(boost::asio::ip::make_address(addr));
    }
    root_peer_.SetRemoteEndpoints(roots, config.port_);
    SetupHandler();
}

void Net::HandleReceive(const boost::system::error_code &error, size_t bytes_received, const std::array<char, 1024> &received_data)
{
    std::cout << "HandleReceive\n";
    if (!error.failed() && bytes_received > 0)
    {
        std::string received_message{
            received_data.begin(),
            received_data.begin() + bytes_received
        };
        if (received_message.find("broadcast") != std::string::npos)
        { 
            std::cout.write("INFO: broadcast message was received", 36);
            std::cout << '\n' << std::flush;
        }
        else if (received_message.find(hostname_) == std::string::npos)
        {
            std::cout.write(received_data.data(), bytes_received);
            std::cout << '\n' << std::flush;
        }
        unicast_peer_.Receive();
    }
}

void Net::SetupHandler()
{
    Handler handler = boost::bind(&Net::HandleReceive,
                                  this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred,
                                  unicast_peer_.GetReceiveBuffer());
    unicast_peer_.SetupReceiver(handler);
}

void Net::Receive()
{
    std::cout << "Receiving\n";
    unicast_peer_.Receive();
    std::cout << "Receiver exit\n";
}

void Net::Send()
{
    bool is_end = false;
    std::string name = hostname_;
    std::string message = "<connected>";
    std::string buffer = name + ": " + message;
    root_peer_.Send(boost::asio::buffer(buffer));
    message.clear();
    while (std::getline(std::cin, message))
    {
        buffer.clear();
        if (message == "-exit")
        {
            message = "<disconnected>";
            is_end = true;
        }
        buffer = name + ": " + message;
        root_peer_.Send(boost::asio::buffer(buffer));
        if (is_end)
        {
            return;
        }
        std::cout << "You: " << message << '\n';
    }
}