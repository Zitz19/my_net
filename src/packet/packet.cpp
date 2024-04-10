#include <cstring>
#include <string>
#include <algorithm>

#include "packet.h"

uint64_t Packet::packet_id_ = 0;

Packet::Packet(const char *sender_ip, const char * receiver_ip, PacketFormat packet_format, bool is_last)
{
    Packet::packet_id_++;
    std::memcpy(sender_ip_, sender_ip, ip_len_);
    sender_ip_[ip_len_] = '\0';
    std::memcpy(receiver_ip_, receiver_ip, ip_len_);
    receiver_ip_[ip_len_] = '\0';
    packet_format_ = packet_format;
    is_last_ = is_last;
}

std::variant<Packet, std::list<Packet>> Packet::SetMessage(const char *message, uint64_t message_size, bool &is_cutted)
{
    uint64_t parts_num = message_size / max_message_size_ + (message_size != max_message_size_);
    is_cutted = parts_num > 1;
    if (!is_cutted)
    {
        std::memcpy(message_, message, message_size);
        std::variant<Packet, std::list<Packet>> var{*this};
        return var;
    } else
    {
        std::list<Packet> packets;
        is_last_ = false;
        packets.push_back(*this);
        for (uint64_t index = 1; index < parts_num - 1; ++index)
        {
            Packet packet(sender_ip_, receiver_ip_, packet_format_, false);
            packets.push_back(std::move(packet));
        }
        Packet packet(sender_ip_, receiver_ip_, packet_format_, true);
        packets.push_back(std::move(packet));
        std::variant<Packet, std::list<Packet>> var{packets};
        return var;
    }
}

std::string FixedLength(uint64_t value, int digits = 5) {
    std::string result;
    while (digits-- > 0) {
        result += ('0' + value % 10);
        value /= 10;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

uint8_t GetOctet(const char *ip_addr, uint8_t octet_index)
{
    uint8_t octet = 0;
    uint8_t start_pos, end_pos;
    for (start_pos = 0; start_pos < std::strlen(ip_addr) && octet_index > 0; ++start_pos)
    {
        if (ip_addr[start_pos] == '.')
        {
            octet_index--;
        }
    }
    for (end_pos = start_pos + 1; end_pos < std::strlen(ip_addr); ++end_pos)
    {
        if (ip_addr[end_pos] == '.')
        {
            break;
        }
    }
    for (uint8_t i = start_pos; i < end_pos; ++i)
    {
        octet *= 10;
        octet += ip_addr[i] - '0';
    }
    return octet;
}

#include <iostream>
char *Packet::ToString()
{
    uint16_t packet_size = 2 * 12 + 1 /* format */ + 5 /* UINT16_MAX length */ + std::strlen(message_);
    char *data = new char[packet_size];

    /* Sender IP */
    std::memcpy(data, FixedLength(GetOctet(sender_ip_, 0), 3).c_str(), 3);
    std::memcpy(data + 3, FixedLength(GetOctet(sender_ip_, 1), 3).c_str(), 3);
    std::memcpy(data + 6, FixedLength(GetOctet(sender_ip_, 2), 3).c_str(), 3);
    std::memcpy(data + 9, FixedLength(GetOctet(sender_ip_, 3), 3).c_str(), 3);

    std::cout << data << std::endl;
    
    /* Format */
    data[ip_len_] = char(packet_format_);

    std::cout << data << std::endl;

    /* Receiver IP */
    std::memcpy(data, FixedLength(GetOctet(receiver_ip_, 0), 3).c_str(), 3);
    std::memcpy(data + 13, FixedLength(GetOctet(receiver_ip_, 1), 3).c_str(), 3);
    std::memcpy(data + 16, FixedLength(GetOctet(receiver_ip_, 2), 3).c_str(), 3);
    std::memcpy(data + 19, FixedLength(GetOctet(receiver_ip_, 3), 3).c_str(), 3);

    std::cout << data << std::endl;
    
    /* Message Size */
    std::memcpy(data + 2 * 12 + 1, FixedLength(std::strlen(message_)).c_str(), 5);

    std::cout << data << std::endl;

    /* Message */
    std::memcpy(data + 2 * 12 + 1 + 5, message_, std::strlen(message_));

    std::cout << data << std::endl;

    return data;
}