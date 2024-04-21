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
        }
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
    receiving_thread_ = std::thread([this] { io_context_.run(); });
    peer_.Receive();
}

void Net::Stop()
{
    peer_.StopReceive();
    receiving_thread_.join();

}

void Net::Send()
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

void Net::SearchNeighbours()
{
    peer_.SearchPeers();
}