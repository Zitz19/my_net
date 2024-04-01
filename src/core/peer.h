#pragma once

#include <boost/asio.hpp>
#include <list>
#include <array>
#include <boost/function.hpp>

typedef boost::function<void (const boost::system::error_code&, size_t bytes_transferred)> Handler;

using boost::asio::ip::udp;

class Peer
{
private:
    udp::endpoint remote_endpoint_;
    std::array<char, 1024> receiving_buffer_;
    Handler handler_;

protected:
    boost::asio::io_context io_context_;
    udp::socket socket_;
    udp::endpoint listen_endpoint_;
    const std::size_t max_datagram_size_ = 1024;

public:
    Peer(boost::asio::ip::address address, uint16_t port);
    Peer(boost::asio::ip::address address, uint16_t port, udp::endpoint remote_endpoint);
    Peer(boost::asio::ip::address address, uint16_t port, boost::asio::ip::address remote_address);

    void SetRemoteEndpoint(boost::asio::ip::address remote_address, uint16_t port);

    const std::array<char, 1024> &GetReceiveBuffer();
    void SetupReceiver(Handler handler);
    void Receive();
};

class MultiPeer: public Peer
{
private:
    std::list<udp::endpoint> remote_endpoints_;

public:
    MultiPeer(boost::asio::ip::address address, uint16_t port);

    void SetRemoteEndpoints(std::list<boost::asio::ip::address> &remote_addresses, uint16_t port);
    void Send(boost::asio::mutable_buffer send_data);
};