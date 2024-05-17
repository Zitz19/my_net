#pragma once

#include <boost/asio.hpp>
#include <list>
#include <array>
#include <boost/function.hpp>
#include <ifaddrs.h>
#include <unordered_map>
#include <bitset>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <atomic>
#include "packet/packet.h"
#include "routing/map.h"
#include "periodic_function.hpp"
#include "VariadicTable.h"

using namespace boost::asio::ip;
using boost::asio::deadline_timer;
namespace posix_time = boost::posix_time;

static const uint8_t kPingCount = 1;

struct PingInfo
{
    std::atomic<uint16_t> num_replies = 0;
    posix_time::time_duration answer_time = posix_time::millisec(0);

    void operator=(const PingInfo &ping_info)
    {
        num_replies = ping_info.num_replies.load();
        answer_time = ping_info.answer_time;
    }
};

/* Можно сделать виртуальный родительский класс интерфейс
 и сделать реализации для всех необходимых протоколов */
class NetInterface
{
public:
    const uint16_t kAddressFamily;
    const address_v4 kIpv4Address;
    const address_v4 kIpv4Mask;

    NetInterface(): kAddressFamily(AF_INET), kIpv4Address(address_v4::from_string("0.0.0.0")), kIpv4Mask(address_v4::from_string("255.255.255.255")) {}
    NetInterface(uint16_t address_family, address_v4 ipv4_address, address_v4 ipv4_mask)
    : kAddressFamily(address_family)
    , kIpv4Address(ipv4_address)
    , kIpv4Mask(ipv4_mask) {}
};

std::unordered_map<std::string, NetInterface> GetAllInterfaces();

bool IsNetContainsAddress(boost::asio::ip::network_v4 const &network, boost::asio::ip::address_v4 const &ip);

class Peer
{
public:
    static const uint32_t receiving_buffer_size_ = 1024;
    static const uint32_t max_datagram_size_ = receiving_buffer_size_;
    static const uint8_t ping_count_ = kPingCount;

private:
    boost::asio::io_context io_context_;
    std::thread receiving_thread_;
    dp::periodic_function<std::_Bind<void (Peer::*(Peer *, bool))(bool)>, dp::policies::schedule_next_missed_interval_policy, void> local_ping_;
    RouteMap map_;
    uint32_t pid_;
    uint16_t port_;
    deadline_timer timer_;
    uint16_t ping_sequence_number_;
    std::array<char, receiving_buffer_size_> receiving_buffer_;
    udp::endpoint sender_ep_;
    std::list<udp::endpoint> roots_;
    std::unordered_map<std::string, NetInterface> interfaces_;
    std::unordered_map<udp::endpoint, PingInfo> ping_stat_;
    std::unordered_map<address_v4, uint32_t> ip_to_pid_;

    void PrintPingStat();

protected:
    udp::socket socket_;
    udp::endpoint listen_endpoint_;

public:
    Peer(address address, uint16_t port);
    void SetPID(uint16_t pid) { pid_ = pid; };
    bool IsMe(address address);
    const std::array<char, receiving_buffer_size_> &receiving_buffer();

    void HandleReceive(const boost::system::error_code &error, size_t bytes_received);
    void StartReceive();
    void Receive();
    void StopReceive();

    address GetSendingInterface(address_v4 receiver_address);
    void SendTo(udp::endpoint remote_ep, Packet packet);
    void SendTo(udp::endpoint remote_ep, std::list<Packet> packets);
    void SendToRoots(std::string message);

    void Ping(udp::endpoint remote_ep, Packet &ping_packet);
    void PingByIP(udp::endpoint remote_ep);
    void AnswerOnPing(udp::endpoint requesting_ep, const Packet &ping_packet);
    void PingRoots();
    void SearchNeighbours(bool remove_inactive = true);

    void SetRoots(std::list<address> &remote_addresses);
    const std::list<udp::endpoint> &GetRoots();

    //void SendAnswerOnSearch(address remote_address, uint16_t port);
};