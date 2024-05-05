#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "app.h"

App::App(Config config)
    : config_(config)
    , peer_(io_context_, address_v4::any(), config.port_)
    , port_(config.port_)
{
    std::list<address> roots;
    for (std::string addr : config.roots_)
    {
        roots.push_back(make_address(addr));
    }
    peer_.SetRemoteEndpoints(roots, config.port_);
    SetupHandler();
}

bool App::CheckRoot(boost::asio::ip::address root)
{

}

void App::HandleReceive(const boost::system::error_code &error, size_t bytes_received)
{
    if (!error.failed() && bytes_received > 0)
    {
        const std::array<char, 1024> &received_data = peer_.GetReceiveBuffer();
        std::string received_message{
            received_data.begin(),
            received_data.begin() + bytes_received
        };
        Packet received_packet = Packet(received_message);
        if (received_packet.format() == PacketFormat::SEARCH and not peer_.IsMe(received_packet.sender_ip()))
        { 
            std::cout << received_packet.Print() << std::flush;
            peer_.SendAnswerOnSearch(boost::asio::ip::make_address(received_packet.sender_ip()), 9012); // config should be used
        }
        else if (received_packet.format() == PacketFormat::STANDART)
        {
            std::cout << received_packet.Print() << std::flush;
        }
        else if (received_packet.format() == PacketFormat::IAMHERE)
        {
            std::cout << received_packet.Print() << std::flush;
            peer_.AddRemoteEndpoint(boost::asio::ip::make_address(received_packet.sender_ip()), port_);
        }
        peer_.Receive();
    }
}

void App::SetupHandler()
{
    Handler handler = boost::bind(&App::HandleReceive,
                                  this,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred);
    peer_.SetupReceiver(handler);
}

void App::Receive()
{
    boost::asio::io_context::work idle_work(io_context_);
    receiving_thread_ = std::thread([this] { io_context_.run(); });
    peer_.Receive();
}

void App::Stop()
{
    peer_.StopReceive();
    receiving_thread_.join();

}

void App::Send()
{
    bool is_end = false;
    std::string message = "<connected>";
    peer_.Send(message);
    while (std::getline(std::cin, message))
    {
        if (message == "-exit")
        {
            message = "<disconnected>";
            is_end = true;
        }
        peer_.Send(message);
        if (is_end)
        {
            return;
        }
        std::cout << "You: " << message << '\n';
    }
}

void App::SearchNeighbours()
{
    peer_.SearchPeers();
}