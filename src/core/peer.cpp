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

void Peer::SetRemoteEndpoints(std::list<boost::asio::ip::address> &remote_addresses, uint16_t port)
{
    for (boost::asio::ip::address addr : remote_addresses)
    {
        remote_endpoints_.push_back(udp::endpoint{addr, port});
    }
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
    socket_.async_receive(
        boost::asio::buffer(receiving_buffer_),
        boost::bind(handler_, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void Peer::StopReceive()
{
    socket_.close();
}

void Peer::RunContext()
{
    io_context_.run();
}

void Peer::Send(boost::asio::mutable_buffer send_data)
{
    for (auto& remote_endpoint : remote_endpoints_)
    {
        socket_.send_to(
            boost::asio::buffer(send_data, max_datagram_size_),
            remote_endpoint
        );
    }
}