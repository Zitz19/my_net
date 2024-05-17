#include <iostream>
#include <cstring>
#include <string>
#include <algorithm>

#include "packet.h"

Packet::Packet(uint32_t receiver_pid, uint32_t sender_pid, const std::string &data, PacketFormat packet_format, bool is_last)
    : receiver_pid_(receiver_pid)
    , sender_pid_(sender_pid)
    , packet_format_(packet_format)
    , is_last_(is_last)
    , data_(data)
{
    if (data.size() > max_message_size_)
    {
        /* Throwing an exeption as a better variant */
        std::cerr << "Data of packet is too long. Please, use another interface!" << std::endl;
        data_ = std::string(data.begin(), data.begin() + max_message_size_);
        data_size_ = max_message_size_;
    } else
    {
        data_ = data;
        data_size_ = data.size();
    }
}

std::list<Packet> Packet::MakePacketList(int32_t receiver_pid, uint32_t sender_pid, const std::string &data, PacketFormat packet_format)
{
    std::list<Packet> packets;
    uint64_t data_size = data.size();
    uint64_t parts_num = data_size / max_message_size_ + (data_size != max_message_size_);
    uint64_t last_packet_size = data_size % max_message_size_;
    for (uint64_t index = 1; index < parts_num - 1; ++index)
    {
        Packet packet(
            receiver_pid, 
            sender_pid, 
            std::string(data.begin() + index * max_message_size_, data.begin() + (index + 1) * max_message_size_), 
            packet_format, 
            false);
        packets.push_back(packet);
    }
    Packet packet(
            receiver_pid, 
            sender_pid, 
            std::string(data.end() - last_packet_size, data.end()), 
            packet_format, 
            true);
    packets.push_back(packet);
    return packets;
}

Packet Packet::MakePacketFromReceivedData(const std::string &data)
{
    uint32_t receiver_pid = (uint32_t)std::atoi(std::string(data.begin(), data.begin() + 5).c_str());
    PacketFormat format = PacketFormat(data[5] - '0');
    uint32_t sender_pid = (uint32_t)std::atoi(std::string(data.begin() + 6, data.begin() + 11).c_str());

    uint16_t offset = 11;
    if (format == PacketFormat::PING or format == PacketFormat::PING_ANSWER)
    {
        address_v4 receiver_ip(std::atoi(std::string(
            data.begin() + offset,
            data.begin() + offset + 10
        ).c_str()));

        offset = 21;
        address_v4 sender_ip(std::atoi(std::string(
            data.begin() + offset,
            data.begin() + offset + 10
        ).c_str()));

        offset = 31;
        posix_time::ptime sent_time = posix_time::time_from_string(
            std::string(data.begin() + offset, data.begin() + offset + timestamp_len_)
        );

        offset += timestamp_len_;
        u_int16_t ping_seq_num = (uint16_t)std::atoi(std::string(data.begin() + offset, data.begin() + offset + 5).c_str());

        offset += 5;
        // Should be removed
        uint16_t data_size;
        data_size = std::atoi(std::string(data.begin() + offset, data.begin() + offset + 5).c_str());
        const std::string &received_data = std::string(data.end() - data_size, data.end());
        Packet packet(receiver_pid, sender_pid, received_data, format, true);
        packet.SetReceiverIP(receiver_ip);
        packet.SetSenderIP(sender_ip);
        packet.SetPingSentTime(sent_time);
        packet.SetPingSequenceNumber(ping_seq_num);
        return packet;
    } else if (format == PacketFormat::STANDART)
    {
        uint16_t data_size;
        data_size = std::atoi(std::string(data.begin() + offset, data.begin() + offset + 5).c_str());
        const std::string &received_data = std::string(data.end() - data_size, data.end());
        Packet packet(receiver_pid, sender_pid, received_data, format, true);
        return packet;
    } else if (format == PacketFormat::RTM_UPD)
    {
        uint32_t src_pid = std::atoi(std::string(
            data.begin() + offset,
            data.begin() + offset + 5
        ).c_str());
        offset = 16;

        uint32_t dst_pid = std::atoi(std::string(
            data.begin() + offset,
            data.begin() + offset + 5
        ).c_str());
        offset = 21;

        uint64_t path_cost = std::atoi(std::string(
            data.begin() + offset,
            data.begin() + offset + 5
        ).c_str());
        Packet packet(receiver_pid, sender_pid, "", format, true);
        packet.SetSrcPID(src_pid);
        packet.SetDstPID(dst_pid);
        packet.SetPathCost(path_cost);
        return packet;
    } else
    {
        std::cerr << "Unkkown packet type." << std::endl;
        return Packet(receiver_pid, sender_pid, "", format, true);
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

std::string Packet::ToString()
{
    std::string packet_str = 
        FixedLength(receiver_pid_, 5) + 
        std::to_string(int(packet_format_)) + 
        FixedLength(sender_pid_, 5);
    if (packet_format_ == PacketFormat::PING or packet_format_ == PacketFormat::PING_ANSWER)
    {
        auto receiver_ip_int = receiver_ip_.to_v4().to_uint();
        auto sender_ip_int = sender_ip_.to_v4().to_uint();
        packet_str += FixedLength(receiver_ip_int, 10);
        packet_str += FixedLength(sender_ip_int, 10);
        packet_str += posix_time::to_simple_string(ping_sent_time_);
        packet_str += FixedLength(ping_sequence_number_, 5);
        packet_str += FixedLength(data_size_, 5) + data_;
    } else if (packet_format_ == PacketFormat::STANDART)
    {
        /* message only here */
        packet_str += FixedLength(data_size_, 5) + data_;
    } else if (packet_format_ == PacketFormat::RTM_UPD)
    {
        packet_str += FixedLength(src_pid_, 5);
        packet_str += FixedLength(dst_pid_, 5);
        packet_str += FixedLength(path_cost_, 5);
    }

    return packet_str;
}

std::string Packet::Print()
{
    std::string buff = "";
    buff += std::string(magic_enum::enum_name(packet_format_)) + " PACKET FORMAT\n";
    buff += "> Receiver PID: " + std::to_string(receiver_pid_) + '\n';
    buff += "> Sender PID: " + std::to_string(sender_pid_) + '\n';
    if (packet_format_ == PacketFormat::STANDART)
    {
        buff += "> Data size: " + std::to_string(data_size_) + ". Data: \n";
        buff += data_ + '\n';
    }
    else if (packet_format_ == PacketFormat::PING or packet_format_ == PacketFormat::PING_ANSWER)
    {
        buff += "> Receiver IP: " + receiver_ip_.to_string() + '\n';
        buff += "> Sender IP: " + sender_ip_.to_string() + '\n';
        buff += "> Ping sequence number: " + std::to_string(ping_sequence_number_) + '\n';
        buff += "> Data size: " + std::to_string(data_size_) + ". Data: \n";
        buff += data_ + '\n';
    } else if (packet_format_ == PacketFormat::RTM_UPD)
    {
        buff += "> Source PID: " + std::to_string(src_pid_) + '\n';
        buff += "> Destination PID: " + std::to_string(dst_pid_) + '\n';
        buff += "> Path cost: " + std::to_string(path_cost_) + '\n';
    }
    return buff;
}