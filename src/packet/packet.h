#include <cstdint>

/**
 * MY PACKET FORMAT
 * 
 * receiver_addr | packet_format | sender_addr
*/

class Packet
{
private:
    static constexpr uint16_t size_ = UINT16_MAX;
    static constexpr uint8_t addr_len_ = 18; // IPv6 max length of address
    static constexpr uint8_t ip_len_ = 16; // IPv4 max length of address
    char sender_addr_[addr_len_];
    char receiver_addr_[addr_len_];
    char sender_ip_[ip_len_];
    char receiver_ip_[ip_len_];
    uint8_t packet_id_;
    uint8_t packet_format_;
};