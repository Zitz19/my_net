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

char *Packet::ToString()
{
    uint16_t packet_size = 2 * ip_len_ + 1 /* format */ + 5 /* UINT16_MAX length */ + std::strlen(message_);
    char *data = new char[packet_size];
    std::memcpy(data, sender_ip_, ip_len_);
    data[ip_len_] = char(packet_format_);
    std::memcpy(data + ip_len_ + 1, receiver_ip_, ip_len_);
    std::memcpy(data + 2 * ip_len_ + 1, FixedLength(std::strlen(message_)).c_str(), 5);
    std::memcpy(data + 2 * ip_len_ + 1 + 5, message_, std::strlen(message_));
    return data;
}