#include <iostream>
#include <boost/bind/bind.hpp>

#include "peer.h"

std::unordered_map<std::string, NetInterface> GetAllInterfaces()
{
    std::unordered_map<std::string, NetInterface> interfaces;

    struct ifaddrs* ptr_ifaddrs = nullptr;

    auto result = getifaddrs(&ptr_ifaddrs);
    if (result != 0)
    {
        std::cerr << "`getifaddrs()` failed: " << strerror(errno) << std::endl;

        return interfaces;
    }

    for (struct ifaddrs* ptr_entry = ptr_ifaddrs; ptr_entry != nullptr; ptr_entry = ptr_entry->ifa_next)
    {
        std::string ipaddress_human_readable_form;
        std::string netmask_human_readable_form;
        std::string netmask_binary_form;

        std::string interface_name = std::string(ptr_entry->ifa_name);
        sa_family_t address_family = ptr_entry->ifa_addr->sa_family;
        if (address_family == AF_INET)
        {
            // IPv4

            // Be aware that the `ifa_addr`, `ifa_netmask` and `ifa_data` fields might contain nullptr.
            // Dereferencing nullptr causes "Undefined behavior" problems.
            // So it is need to check these fields before dereferencing.
            if (ptr_entry->ifa_addr != nullptr)
            {
                char buffer[INET_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in*)(ptr_entry->ifa_addr))->sin_addr,
                    buffer,
                    INET_ADDRSTRLEN
                );

                ipaddress_human_readable_form = std::string(buffer);
            }

            if (ptr_entry->ifa_netmask != nullptr){
                char buffer[INET_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in*)(ptr_entry->ifa_netmask))->sin_addr,
                    buffer,
                    INET_ADDRSTRLEN
                );

                netmask_human_readable_form = std::string(buffer);
            }

            std::cout << interface_name << ": IP address = " << ipaddress_human_readable_form << ", netmask = " << netmask_human_readable_form << std::endl;

            uint8_t subnet_octets[4];
            uint8_t short_netmask = 0;

            for (uint8_t i = 0; i < 4; ++i)
            {
                subnet_octets[i] = GetOctet(netmask_human_readable_form.c_str(), i);
                uint8_t reversed_octet = ~subnet_octets[i];
                uint8_t ones_num = 8;
                while (reversed_octet > 0)
                {
                    ones_num -= reversed_octet % 2;
                    reversed_octet /= 2;
                }
                short_netmask += ones_num;
            }

            std::string subnet = std::to_string(GetOctet(ipaddress_human_readable_form.c_str(), 0) & subnet_octets[0]) + '.'
                + std::to_string(GetOctet(ipaddress_human_readable_form.c_str(), 1) & subnet_octets[1]) + '.'
                + std::to_string(GetOctet(ipaddress_human_readable_form.c_str(), 2) & subnet_octets[2]) + '.'
                + std::to_string(GetOctet(ipaddress_human_readable_form.c_str(), 3) & subnet_octets[3]);
            interfaces.emplace(subnet, NetInterface(address_family, ipaddress_human_readable_form, netmask_human_readable_form, std::to_string(short_netmask)));
        }
#if 0
        // Disabled temporary
        else if (address_family == AF_INET6)
        {
            // IPv6
            uint32_t scope_id = 0;
            if (ptr_entry->ifa_addr != nullptr)
            {
                char buffer[INET6_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in6*)(ptr_entry->ifa_addr))->sin6_addr,
                    buffer,
                    INET6_ADDRSTRLEN
                );

                ipaddress_human_readable_form = std::string(buffer);
                scope_id = ((struct sockaddr_in6*)(ptr_entry->ifa_addr))->sin6_scope_id;
            }

            if (ptr_entry->ifa_netmask != nullptr)
            {
                char buffer[INET6_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in6*)(ptr_entry->ifa_netmask))->sin6_addr,
                    buffer,
                    INET6_ADDRSTRLEN
                );

                netmask_human_readable_form = std::string(buffer);
            }

            std::cout << interface_name << ": IP address = " << ipaddress_human_readable_form << ", netmask = " << netmask_human_readable_form << ", Scope-ID = " << scope_id << std::endl;
        }
#endif /* 0 */
        else {
            // AF_UNIX, AF_UNSPEC, AF_PACKET etc.
            // If ignored, delete this section.
        }
    }

    freeifaddrs(ptr_ifaddrs);
    return interfaces;
}

bool IsNetContainsAddress(boost::asio::ip::network_v4 const &network, boost::asio::ip::address_v4 const &ip)
{
    return network.canonical().address() ==
        boost::asio::ip::network_v4(ip, network.prefix_length()).canonical().address();
}

Peer::Peer(boost::asio::io_context &io_context, address address, uint16_t port)
    : socket_{io_context}
    , listen_endpoint_{address, port}
{
    socket_.open(boost::asio::ip::udp::v4());
    socket_.set_option(udp::socket::reuse_address(true));
    socket_.bind(listen_endpoint_);
    interfaces_ = GetAllInterfaces();
}

void Peer::SetRemoteEndpoints(std::list<boost::asio::ip::address> &remote_addresses, uint16_t port)
{
    for (boost::asio::ip::address addr : remote_addresses)
    {
        remote_endpoints_.push_back(udp::endpoint{addr, port});
    }
}

const std::array<char, 1024> &Peer::GetReceiveBuffer()
{
    return receiving_buffer_;
}

void Peer::SetupReceiver(Handler handler)
{
    handler_ = handler;
}

void Peer::Receive()
{
    socket_.async_receive(
        boost::asio::buffer(receiving_buffer_),
        boost::bind(handler_, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void Peer::StopReceive()
{
    socket_.close();
}

void Peer::Send(std::string send_data)
{
    for (auto& remote_endpoint : remote_endpoints_)
    {
        std::string sending_address;
        for (auto &interface : interfaces_)
        {
            if (IsNetContainsAddress(boost::asio::ip::make_network_v4(interface.first + '/' + interface.second.kIpv4ShortMask), remote_endpoint.address().to_v4()))
            {
                sending_address = interface.second.kIpv4Address;
                break;
            }
        }
        Packet packet = Packet(sending_address.c_str(), remote_endpoint.address().to_string().c_str());
        bool is_cutted;
        std::variant var = packet.SetMessage(send_data.c_str(), send_data.size(), is_cutted);
        if (!is_cutted)
        {
            socket_.send_to(
                boost::asio::buffer(std::string(std::get<Packet>(var).ToString()), max_datagram_size_),
                remote_endpoint
            );
        } else
        {
            for (auto &p : std::get<std::list<Packet>>(var))
            {
                socket_.send_to(
                    boost::asio::buffer(std::string(p.ToString()), max_datagram_size_),
                    remote_endpoint
                );
            }
        }
    }
}