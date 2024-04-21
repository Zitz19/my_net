#include <cstdint>
#include <variant>
#include <list>
#include "magic_enum.hpp"

/**
 * MY PACKET FORMAT
 * 
 * receiver_addr | packet_format | sender_addr | size | message
*/

enum class PacketFormat
{
    SEARCH = 1,
    IAMHERE = 2,
    STANDART = 3,
    INFO = 4
};

class Packet
{
private:
    static constexpr uint16_t size_ = 1500;
    static constexpr uint8_t addr_len_ = 17; // IPv6 max length of address
    static constexpr uint8_t ip_len_ = 15; // IPv4 max length of address
    static constexpr uint16_t max_message_size_ = 100;
    char sender_addr_[addr_len_+1];
    char receiver_addr_[addr_len_+1];
    char sender_ip_[ip_len_+1];
    char receiver_ip_[ip_len_+1];
    static uint64_t packet_id_;
    PacketFormat packet_format_;
    char message_[max_message_size_];
    uint16_t message_size_ = 0;
    bool is_last_;
public:
    Packet(const char *sender_ip, const char * receiver_ip, PacketFormat packet_format = PacketFormat::STANDART, bool is_last = true);
    std::variant<Packet, std::list<Packet>> SetMessage(const char *message, uint64_t message_size, bool &is_cutted);
    char *ToString();
    Packet(const std::string &received_data);
    std::string Print();
    PacketFormat format() { return packet_format_; };
    std::string sender_ip() { return std::string(sender_ip_); };
};

std::string FixedLength(uint64_t value, int digits = 5);
uint8_t GetOctet(const char *ip_addr, uint8_t octet_index);