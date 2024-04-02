#include <iostream>
#include <boost/bind/bind.hpp>

#include "peer.h"

Peer::Peer(boost::asio::ip::address address, uint16_t port)
    : socket_{io_context_}
    , listen_endpoint_{address, port}
{
    socket_.open(boost::asio::ip::udp::v4());
    socket_.set_option(udp::socket::reuse_address(true));
    socket_.bind(listen_endpoint_);
}

Peer::Peer(boost::asio::ip::address address, uint16_t port, udp::endpoint remote_endpoint)
    : Peer(address, port)
{
    remote_endpoint_ = remote_endpoint;
}

Peer::Peer(boost::asio::ip::address address, uint16_t port, boost::asio::ip::address remote_address)
    : Peer(address, port)
{
    remote_endpoint_ = udp::endpoint{remote_address, port};
}

void Peer::SetRemoteEndpoint(boost::asio::ip::address remote_address, uint16_t port)
{
    remote_endpoint_ = udp::endpoint{remote_address, port};
}

const std::array<char, 1024> &Peer::GetReceiveBuffer()
{
    return receiving_buffer_;
}

void Peer::SetupReceiver(Handler handler)
{
    handler_ = handler;
}

void Peer::Receive()
{
    std::cout << "Receive\n";
    socket_.async_receive(
        boost::asio::buffer(receiving_buffer_),
        // boost::bind(handler_, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
        [this](const boost::system::error_code& error_code,
        std::size_t bytes_received)
        {
            std::cout << "Handler\n";
            if (!error_code.failed() && bytes_received > 0)
            {
                std::string received_message{
                    receiving_buffer_.begin(),
                    receiving_buffer_.begin() + bytes_received
                };
                if (received_message.find("broadcast") != std::string::npos)
                { 
                    std::cout.write("INFO: broadcast message was received", 36);
                    std::cout << '\n' << std::flush;
                }
                else
                {
                    std::cout.write(receiving_buffer_.data(), bytes_received);
                    std::cout << '\n' << std::flush;
                }
                Receive();
            } else
            {
                std::cout << error_code.message() << std::endl;
            }
        }
    );
}

void Peer::RunContext()
{
    io_context_.run();
}

MultiPeer::MultiPeer(boost::asio::ip::address address, uint16_t port)
    : Peer(address, port)
{}

void MultiPeer::SetRemoteEndpoints(std::list<boost::asio::ip::address> &remote_addresses, uint16_t port)
{
    for (boost::asio::ip::address addr : remote_addresses)
    {
        remote_endpoints_.push_back(udp::endpoint{addr, port});
    }
}

void MultiPeer::Send(boost::asio::mutable_buffer send_data)
{
    for (auto& remote_endpoint : remote_endpoints_)
    {
        socket_.send_to(
            boost::asio::buffer(send_data, max_datagram_size_),
            remote_endpoint
        );
    }
    socket_.send_to(
            boost::asio::buffer(send_data, max_datagram_size_),
            listen_endpoint_
        );
}