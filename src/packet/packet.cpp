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
        message_size_ = message_size;
        std::memcpy(message_, message, message_size);
        std::variant<Packet, std::list<Packet>> var{*this};
        return var;
    } else
    {
        std::list<Packet> packets;
        is_last_ = false;
        message_size_ = max_message_size_;
        std::memcpy(message_, message, max_message_size_);
        packets.push_back(*this);
        for (uint64_t index = 1; index < parts_num - 1; ++index)
        {
            Packet packet(sender_ip_, receiver_ip_, packet_format_, false);
            std::memcpy(packet.message_, message + index * max_message_size_, max_message_size_);
            packet.message_size_ = max_message_size_;
            packets.push_back(std::move(packet));
        }
        Packet packet(sender_ip_, receiver_ip_, packet_format_, true);
        packet.message_size_ = message_size % max_message_size_;
        std::memcpy(packet.message_, message + (parts_num - 1) * max_message_size_, packet.message_size_);
        packets.push_back(std::move(packet));
        std::variant<Packet, std::list<Packet>> var{packets};
        return var;
    }
}

std::string FixedLength(uint64_t value, int digits) 
{
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

char *Packet::ToString()
{
    uint16_t message_size = message_size_;
    uint16_t packet_size = 2 * 12 + 1 /* format */ + 5 /* UINT16_MAX length */ + message_size + 1;
    char *data = new char[packet_size];

    /* Sender IP */
    std::memcpy(data, FixedLength(GetOctet(sender_ip_, 0), 3).c_str(), 3);
    std::memcpy(data + 3, FixedLength(GetOctet(sender_ip_, 1), 3).c_str(), 3);
    std::memcpy(data + 6, FixedLength(GetOctet(sender_ip_, 2), 3).c_str(), 3);
    std::memcpy(data + 9, FixedLength(GetOctet(sender_ip_, 3), 3).c_str(), 3);
    
    /* Format */
    data[12] = char(packet_format_);

    /* Receiver IP */
    std::memcpy(data + 13, FixedLength(GetOctet(receiver_ip_, 0), 3).c_str(), 3);
    std::memcpy(data + 16, FixedLength(GetOctet(receiver_ip_, 1), 3).c_str(), 3);
    std::memcpy(data + 19, FixedLength(GetOctet(receiver_ip_, 2), 3).c_str(), 3);
    std::memcpy(data + 22, FixedLength(GetOctet(receiver_ip_, 3), 3).c_str(), 3);
    
    /* Message Size */
    std::memcpy(data + 2 * 12 + 1, FixedLength(message_size).c_str(), 5);

    /* Message */
    std::memcpy(data + 2 * 12 + 1 + 5, message_, message_size);
    data[2 * 12 + 1 + 5 + message_size] = '\0';

    return data;
}

Packet::Packet(const std::string &received_data)
{
    uint32_t pos = 0;
    std::string sender_ip = "";
    for (uint8_t octet_index = 0; octet_index < 4; octet_index++)
    {
        sender_ip += std::to_string(std::atoi(received_data.substr(pos, 3).c_str()));
        sender_ip += octet_index < 3 ? "." : "";
        pos += 3;
    }
    PacketFormat format = (PacketFormat) received_data[pos];
    pos += 1;
    std::string receiver_ip = "";
    for (uint8_t octet_index = 0; octet_index < 4; octet_index++)
    {
        receiver_ip += std::to_string(std::atoi(received_data.substr(pos, 3).c_str()));
        receiver_ip += octet_index < 3 ? "." : "";
        pos += 3;
    }
    uint16_t message_size = std::atoi(received_data.substr(pos, 5).c_str());
    pos += 5;
    std::string message = received_data.substr(pos, message_size);
    
    std::memcpy(sender_ip_, sender_ip.c_str(), sender_ip.size());
    std::memcpy(receiver_ip_, receiver_ip.c_str(), receiver_ip.size());
    message_size_ = message_size;
    packet_format_ = format;
    std::memcpy(message_, message.c_str(), message.size());
}

std::string Packet::Print()
{
    std::string buff = "";
    buff += std::string(magic_enum::enum_name(packet_format_)) + " PACKET FORMAT\n";
    buff += "> Receiver IP: " + std::string(receiver_ip_) + '\n';
    buff += "> Sender IP: " + std::string(sender_ip_) + '\n';
    if (packet_format_ == PacketFormat::SEARCH or packet_format_ == PacketFormat::IAMHERE)
    {
        return buff;
    }
    buff += "> Data size: " + std::to_string(message_size_) + ". Data: \n";
    buff += std::string(message_, message_size_) + '\n';
    return buff;
}