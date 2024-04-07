#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "client.h"

Net::Net(Config config)
    : hostname_(config.name_)
    , peer_(io_context_, address_v4::any(), config.port_)
{
    std::list<address> roots;
    for (std::string addr : config.roots_)
    {
        roots.push_back(make_address(addr));
    }
    peer_.SetRemoteEndpoints(roots, config.port_);
    SetupHandler();
}

void Net::HandleReceive(const boost::system::error_code &error, size_t bytes_received)
{
    if (!error.failed() && bytes_received > 0)
    {
        const std::array<char, 1024> &received_data = peer_.GetReceiveBuffer();
        std::string received_message{
            received_data.begin(),
            received_data.begin() + bytes_received
        };
        if (received_message.find("broadcast") != std::string::npos)
        { 
            std::cout.write("INFO: broadcast message was received", 36);
            std::cout << '\n' << std::flush;
        } else if (received_message.find(hostname_) == std::string::npos)
        {
            std::cout.write(received_data.data(), bytes_received);
            std::cout << '\n' << std::flush;
        }
        std:: cout << peer_.GetAnswersQueue().front() << std::endl;
        peer_.GetAnswersQueue().pop();
        peer_.Receive();
    }
}

void Net::SetupHandler()
{
    Handler handler = boost::bind(&Net::HandleReceive,
                                  this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred);
    peer_.SetupReceiver(handler);
}

void Net::Receive()
{
    boost::asio::io_context::work idle_work(io_context_);
    main_thread_ = std::thread([this] { io_context_.run(); });
    peer_.Receive();
}

void Net::Stop()
{
    peer_.StopReceive();
    main_thread_.join();

}

void Net::Send()
{
    bool is_end = false;
    std::string name = hostname_;
    std::string message = "<connected>";
    std::string buffer = name + ": " + message;
    peer_.Send(boost::asio::buffer(buffer));
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
        peer_.Send(boost::asio::buffer(buffer));
        if (is_end)
        {
            return;
        }
        std::cout << "You: " << message << '\n';
    }
}