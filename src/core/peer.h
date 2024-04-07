#pragma once

#include <boost/asio.hpp>
#include <list>
#include <array>
#include <queue>
#include <boost/function.hpp>
#include "packet/packet.h"

typedef boost::function<void (const boost::system::error_code&, size_t bytes_transferred)> Handler;

using namespace boost::asio::ip;

class Peer
{
private:
    std::list<udp::endpoint> remote_endpoints_;
    std::array<char, 1024> receiving_buffer_;
    Handler handler_;
    std::queue<udp::endpoint> answers_queue_;


protected:
    udp::socket socket_;
    udp::endpoint listen_endpoint_;
    const std::size_t max_datagram_size_ = 1500;

public:
    Peer(boost::asio::io_context &io_context, address address, uint16_t port);

    void SetRemoteEndpoints(std::list<address> &remote_addresses, uint16_t port);
    void Send(std::string send_data);

    const std::array<char, 1024> &GetReceiveBuffer();
    std::queue<udp::endpoint> &GetAnswersQueue();
    void SetupReceiver(Handler handler);
    void Receive();
    void StopReceive();
};