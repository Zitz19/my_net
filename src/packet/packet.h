#pragma once

#include <cstdint>
#include <variant>
#include <list>
#include "magic_enum.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::asio::ip;
namespace posix_time = boost::posix_time;

/**
 * PACKET FORMAT
 * receiver_pid | packet_format | sender_pid | size | message
 * 
 * PING PACKET
 * receiver_pid | packet_format | sender_pid | receiver_ip | sender_ip | timestamp | ping_sequence_number_
 * 
 * RTM_UPD PACKET
 * receiver_pid | packet_format | sender_pid | src_pid | dst_pid| path_cost
*/

enum class PacketFormat
{
    PING = 1,
    PING_ANSWER = 2,
    STANDART = 3,
    HELLO = 4,
    RTM_UPD = 5 /* RouteMap Update */
};

class Packet
{
private:
    static constexpr uint16_t size_ = 1500;
    static constexpr uint8_t ip_len_ = 15; // IPv4 max length of address
    static constexpr uint16_t max_message_size_ = 1024;
    uint32_t sender_pid_;
    uint32_t receiver_pid_;
    address sender_ip_;
    address receiver_ip_;
    PacketFormat packet_format_;
    std::string data_;
    uint16_t data_size_;
    bool is_last_;
    posix_time::ptime ping_sent_time_;
    static const uint8_t timestamp_len_ = 27;
    uint32_t ping_sequence_number_;
    uint32_t src_pid_;
    uint32_t dst_pid_;
    uint64_t path_cost_;
public:
    Packet(uint32_t receiver_pid, uint32_t sender_pid, const std::string &data, PacketFormat packet_format = PacketFormat::STANDART, bool is_last = true);
    static std::list<Packet> MakePacketList(int32_t receiver_pid, uint32_t sender_pid, const std::string &data, PacketFormat packet_format = PacketFormat::STANDART);
    static Packet MakePacketFromReceivedData(const std::string &data);
    std::string ToString();
    std::string Print();
    void SetReceiverPID(uint32_t receiver_pid) { receiver_pid_ = receiver_pid; }
    uint32_t receiver_pid() const { return receiver_pid_; }
    void SetSenderPID(uint32_t sender_pid) { sender_pid_ = sender_pid; }
    uint32_t sender_pid() const { return sender_pid_; }
    PacketFormat format() const { return packet_format_; };
    address receiver_ip() const { return receiver_ip_; };
    address sender_ip() const { return sender_ip_; };
    void SetReceiverIP( const address &receiver_ip) { receiver_ip_ = receiver_ip; }; 
    void SetSenderIP( const address &sender_ip) { sender_ip_ = sender_ip; };
    void SetPingSentTime(posix_time::ptime current_time) { ping_sent_time_ = current_time; }
    posix_time::ptime ping_sent_time() const { return ping_sent_time_; }
    void SetPingSequenceNumber(uint16_t seq_num) { ping_sequence_number_ = seq_num; }
    uint16_t ping_sequence_number() const { return ping_sequence_number_; }
    uint32_t src_pid() const { return src_pid_; }
    void SetSrcPID(uint32_t src_pid) { src_pid_ = src_pid; }
    uint32_t dst_pid() const { return dst_pid_; }
    void SetDstPID(uint32_t dst_pid) { dst_pid_ = dst_pid; }
    uint64_t path_cost() const { return path_cost_; }
    void SetPathCost(uint64_t path_cost) { path_cost_ = path_cost; }
};

std::string FixedLength(uint64_t value, int digits = 5);