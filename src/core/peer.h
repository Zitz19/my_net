#pragma once

#include <boost/asio.hpp>
#include <list>
#include <array>
#include <boost/function.hpp>
#include <ifaddrs.h>
#include <unordered_map>
#include <bitset>
#include "packet/packet.h"
#include "routing/map.h"

/* Можно сделать виртуальный родительский класс интерфейс
 и сделать реализации для всех необходимых протоколов */
class NetInterface
{
public:
    const uint16_t kAddressFamily;
    const std::string kIpv4Address;
    const std::string kIpv4FullMask;
    const std::string kIpv4ShortMask;

    NetInterface(uint16_t address_family, std::string ipv4_address, std::string ipv4_full_mask, std::string ipv4_short_mask)
    : kAddressFamily(address_family)
    , kIpv4Address(ipv4_address)
    , kIpv4FullMask(ipv4_full_mask)
    , kIpv4ShortMask(ipv4_short_mask) {}
};

std::unordered_map<std::string, NetInterface> GetAllInterfaces();

bool IsNetContainsAddress(boost::asio::ip::network_v4 const &network, boost::asio::ip::address_v4 const &ip);

typedef boost::function<void (const boost::system::error_code&, size_t bytes_transferred)> Handler;

using namespace boost::asio::ip;

class Peer
{
private:
    Route

    std::list<udp::endpoint> remote_endpoints_;
    std::array<char, 1024> receiving_buffer_;
    Handler handler_;
    std::unordered_map<std::string, NetInterface> interfaces_;
    uint16_t port_;

public:
    boost::asio::io_context io_context_;
protected:
    udp::socket socket_;
    udp::endpoint listen_endpoint_;
    const std::size_t max_datagram_size_ = 1024;

public:
    Peer(boost::asio::io_context &io_context, address address, uint16_t port);

    void SetRemoteEndpoints(std::list<address> &remote_addresses, uint16_t port);
    void Send(std::string send_data);
    void SendTo(address remote_address, std::string send_data, PacketFormat format);
    void SearchPeers();
    void SendAnswerOnSearch(address remote_address, uint16_t port);
    void AddRemoteEndpoint(boost::asio::ip::address remote_address, uint16_t port);
    bool IsMe(std::string address);

    const std::array<char, 1024> &GetReceiveBuffer();
    void SetupReceiver(Handler handler);
    void Receive();
    void StopReceive();
};