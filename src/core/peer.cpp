#include <iostream>
#include <boost/bind/bind.hpp>

#include "peer.h"

Peer::Peer(boost::asio::io_context &io_context, address address, uint16_t port)
    : socket_{io_context}
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

std::queue<udp::endpoint> &Peer::GetAnswersQueue()
{
    return answers_queue_;
}

void Peer::SetupReceiver(Handler handler)
{
    handler_ = handler;
}

void Peer::Receive()
{
    udp::endpoint sender_ep;
    answers_queue_.push(sender_ep);
    socket_.async_receive_from(
        boost::asio::buffer(receiving_buffer_),
        sender_ep,
        boost::bind(handler_, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void Peer::StopReceive()
{
    socket_.close();
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