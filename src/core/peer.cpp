#include <iostream>
#include <boost/bind/bind.hpp>

#include "peer.h"

std::unordered_map<std::string, NetInterface> GetAllInterfaces()
{
    std::unordered_map<std::string, NetInterface> interfaces = {};

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

            interfaces.emplace(
                network_v4(address_v4::from_string(ipaddress_human_readable_form), address_v4::from_string(netmask_human_readable_form)).to_string(), 
                NetInterface(address_family, address_v4::from_string(ipaddress_human_readable_form), address_v4::from_string(netmask_human_readable_form))
            );
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

/* ===================== Peer methods ========================== */

Peer::Peer(address address, uint16_t port)
    : socket_{io_context_}
    , timer_{io_context_}
    , listen_endpoint_{address, port}
    , port_(port)
    , local_ping_{dp::periodic_function(std::bind(&Peer::SearchNeighbours, this, true), std::chrono::seconds(10))}
{
    socket_.open(boost::asio::ip::udp::v4());
    socket_.set_option(udp::socket::reuse_address(true));
    socket_.set_option(boost::asio::socket_base::broadcast(true));
    socket_.bind(listen_endpoint_);
    interfaces_ = GetAllInterfaces();
}

bool Peer::IsMe(address address)
{
    for (auto &interface : interfaces_)
    {
        if (interface.second.kIpv4Address == address)
        {
            return true;
        }
    }
    return false;
}

const std::array<char, Peer::receiving_buffer_size_> &Peer::receiving_buffer()
{
    return receiving_buffer_;
}

/* ======================== Receive methods ============================= */

void Peer::StartReceive()
{
    boost::asio::io_context::work idle_work(io_context_);
    receiving_thread_ = std::thread([this] { io_context_.run(); });
    Receive();
    local_ping_.start();
    SearchNeighbours(false);
}

void Peer::HandleReceive(const boost::system::error_code &error, size_t bytes_received)
{
    if (!error.failed() && bytes_received > 0)
    {;
        std::string received_message{
            receiving_buffer_.begin(),
            receiving_buffer_.begin() + bytes_received
        };
        Packet received_packet = Packet::MakePacketFromReceivedData(received_message);
        // std::cout << "RECEIVED MESSAGE FROM: " << sender_ep_ << std::endl;
        if (received_packet.format() == PacketFormat::PING and not IsMe(received_packet.sender_ip()))
        { 
            std::cout << received_packet.Print() << std::flush;
            AnswerOnPing(sender_ep_, received_packet);
            // SendAnswerOnSearch(received_packet.sender_ip(), port_);
        } else if (received_packet.format() == PacketFormat::PING) /* Ping from by yourself */
        {
            /* Ignore */
        }
        else if (received_packet.format() == PacketFormat::STANDART)
        {
            std::cout << received_packet.Print() << std::flush;
        }
        else if (received_packet.format() == PacketFormat::PING_ANSWER)
        {
            std::cout << received_packet.Print() << std::flush;
            std::cout << "Ping time: ";
            std:: cout << (
                posix_time::microsec_clock::universal_time() - received_packet.ping_sent_time()
            ).total_milliseconds() << "ms" << std::endl;

            //std::cout << "BEFORE ANSWER ANALYSIS:\n";
            //PrintPingStat();
            
            udp::endpoint answering_ep = udp::endpoint(received_packet.sender_ip(), port_);
            if (ping_stat_.find(answering_ep) == ping_stat_.end())
            {
                ping_stat_[answering_ep] = PingInfo();
                ping_stat_[answering_ep].num_replies.store(1);
                ping_stat_[answering_ep].answer_time = posix_time::microsec_clock::universal_time() - received_packet.ping_sent_time();
                ip_to_pid_[answering_ep.address().to_v4()] = received_packet.sender_pid();
            } else
            {
                ping_stat_[answering_ep].num_replies += 1;
                ping_stat_[answering_ep].answer_time = posix_time::microsec_clock::universal_time() - received_packet.ping_sent_time();
                ip_to_pid_[answering_ep.address().to_v4()] = received_packet.sender_pid();
            }
            Packet rtm_upd_packet = map_.UpdateLink(received_packet.sender_pid(), received_packet.receiver_pid(), RouteMap::CalculatePathCostfromTime(ping_stat_[answering_ep].answer_time));
            rtm_upd_packet.SetSenderPID(pid_);
            /* Send routemap update */
            for (const auto &neighbour : ping_stat_)
            {
                rtm_upd_packet.SetReceiverPID(ip_to_pid_[neighbour.first.address().to_v4()]);
                SendTo(neighbour.first, rtm_upd_packet);
            }
            // std::cout << "CHECK PING_ANSW: " << answering_ep << " REPLIES: " << ping_stat_[answering_ep].num_replies << std::endl;
        } else if (received_packet.format() == PacketFormat::RTM_UPD)
        {
            // std::cout << received_packet.Print() << std::flush;
            if (received_packet.path_cost() != map_.GetPathCost(received_packet.src_pid(), received_packet.dst_pid()))
            {
                map_.UpdateLink(received_packet.src_pid(), received_packet.dst_pid(), received_packet.path_cost());
                // network_v4 net_of_sender_pid;
                // for (auto interface : interfaces_)
                // {
                //     net_of_sender_pid = make_network_v4(interface.first);
                //     if (IsNetContainsAddress(net_of_sender_pid, sender_ep_.address().to_v4()))
                //     {
                //         break;
                //     }
                // }
                // received_packet.SetSenderPID(pid_);
                // for (const auto &neighbour : ping_stat_)
                // {
                //     if (not IsNetContainsAddress(net_of_sender_pid, neighbour.first.address().to_v4()))
                //     {
                //         received_packet.SetReceiverPID(ip_to_pid_[neighbour.first.address().to_v4()]);
                //         SendTo(neighbour.first, received_packet);
                //     }
                // }
            }
        } else
        {
            std::cout << "RECEIVED: " << received_message << std::endl;
        }
        Receive();
    }
}

void Peer::Receive()
{
    socket_.async_receive_from(
        boost::asio::buffer(receiving_buffer_),
        sender_ep_,
        boost::bind(&Peer::HandleReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void Peer::StopReceive()
{
    local_ping_.stop();
    socket_.close();
    receiving_thread_.join();
}

/* ============================ Send methods ==================================*/

address Peer::GetSendingInterface(address_v4 receiver_address)
{
    for (auto interface : interfaces_)
    {
        if (IsNetContainsAddress(make_network_v4(interface.first), receiver_address))
        {
            return interface.second.kIpv4Address;
        }
    }
    throw std::invalid_argument("Interface for address " + receiver_address.to_string() + " not found.");
}

void Peer::SendTo(udp::endpoint remote_ep, Packet packet)
{
    socket_.send_to(
        boost::asio::buffer(packet.ToString(), max_datagram_size_),
        remote_ep
    );
}

void Peer::SendTo(udp::endpoint remote_ep, std::list<Packet> packets)
{
    for (auto &packet : packets)
    {
        socket_.send_to(
            boost::asio::buffer(packet.ToString(), max_datagram_size_),
            remote_ep
        );
    }
}

void Peer::SendToRoots(std::string message)
{
    for (const auto &root : roots_)
    {        
        for (auto &packet : Packet::MakePacketList(0, pid_, message, PacketFormat::STANDART))
        {
            socket_.send_to(
                boost::asio::buffer(packet.ToString(), max_datagram_size_), 
                udp::endpoint(root.address(), port_)
            );
        }
    } 
}

/* ================= Ping methods ====================== */

void Peer::Ping(udp::endpoint remote_ep, Packet &ping_packet)
{
    ping_packet.SetPingSentTime(posix_time::microsec_clock::universal_time());
    socket_.send_to(
        boost::asio::buffer(ping_packet.ToString(), max_datagram_size_), 
        remote_ep
    );
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void Peer::PingByIP(udp::endpoint remote_ep)
{
    Packet ping_packet(0x00, pid_, std::string("ping message"), PacketFormat::PING);
    try
    {
        ping_packet.SetSenderIP(GetSendingInterface(remote_ep.address().to_v4()));
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        ping_packet.SetSenderIP(make_address("0.0.0.0"));
    }
    ping_packet.SetReceiverIP(remote_ep.address());
    // ping_packet.SetPingSentTime(posix_time::microsec_clock::universal_time());
    // socket_.send_to(
    //     boost::asio::buffer(ping_packet.ToString(), max_datagram_size_), 
    //     remote_ep
    // );
    Ping(remote_ep, ping_packet);
}

void Peer::AnswerOnPing(udp::endpoint requesting_ep, const Packet &ping_packet)
{
    Packet ping_answer_packet(ping_packet.sender_pid(), pid_, std::string("ping answer"), PacketFormat::PING_ANSWER);
    bool is_broadcast = false;
    address sender_ip;
    for (auto interface : interfaces_)
    {
        network_v4 interface_net = make_network_v4(interface.first);
        if (interface_net.broadcast() == ping_packet.receiver_ip().to_v4())
        {
            is_broadcast = true;
            sender_ip = interface.second.kIpv4Address;
        }
    }
    if (is_broadcast)
    {
        ping_answer_packet.SetSenderIP(sender_ip);
    } else
    {
        ping_answer_packet.SetSenderIP(ping_packet.receiver_ip());
    }
    ping_answer_packet.SetReceiverIP(requesting_ep.address());
    ping_answer_packet.SetPingSentTime(ping_packet.ping_sent_time());
    socket_.send_to(
        boost::asio::buffer(ping_answer_packet.ToString(), max_datagram_size_), 
        requesting_ep
    );
}

void Peer::PingRoots()
{
    for (const auto &root : roots_)
    {
        Packet ping_packet(0x00, pid_, std::string("ping message"), PacketFormat::PING);
        address sending_address;
        try
        {
            address sending_address = GetSendingInterface(root.address().to_v4());
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            sending_address = make_address("0.0.0.0");
        }
        ping_packet.SetSenderIP(sending_address);
        ping_packet.SetReceiverIP(root.address());
        // ping_packet.SetPingSentTime(posix_time::microsec_clock::universal_time());
        // socket_.send_to(
        //     boost::asio::buffer(ping_packet.ToString(), max_datagram_size_), 
        //     udp::endpoint{root.address(), port_}
        // );
        Ping(udp::endpoint{root.address(), port_}, ping_packet);
    }
}

void Peer::PrintPingStat()
{
    VariadicTable<address, uint16_t, uint64_t> vt({"Address", "PID", "Time"}, 20);
    for (const auto &neighbour : ping_stat_)
    {
        vt.addRow(neighbour.first.address(), ip_to_pid_[neighbour.first.address().to_v4()], neighbour.second.answer_time.total_milliseconds());
    }
    vt.print(std::cout);
}

void Peer::SearchNeighbours(bool remove_inactive)
{
    if (remove_inactive)
    {
        auto neighbour = ping_stat_.begin();
        while (neighbour != ping_stat_.end())
        {
            if (neighbour->second.num_replies.load() == 0)
            {
                //std::cout << "CHECK ERASE: " << neighbour->first << std::endl;
                /* Remove neighbour */
                uint32_t removed_pid = ip_to_pid_[neighbour->first.address().to_v4()];
                Packet rtm_upd_packet = map_.UpdateLink(pid_, removed_pid, 0);
                std::cout << "--REMOVED PID: " << removed_pid << std::endl;
                uint64_t paths_sum = 0;
                for (auto connection : map_.GetLinkStateTable()[removed_pid])
                {
                    paths_sum += connection.second;
                }
                if (paths_sum == 0)
                {
                    map_.RemovePID(removed_pid);
                }
                ip_to_pid_.erase(neighbour->first.address().to_v4());
                ping_stat_.erase(neighbour++);
                PrintPingStat();
                rtm_upd_packet.SetSenderPID(pid_);
                /* Send routemap update */
                for (const auto &neighbour : ping_stat_)
                {
                    rtm_upd_packet.SetReceiverPID(ip_to_pid_[neighbour.first.address().to_v4()]);
                    SendTo(neighbour.first, rtm_upd_packet);
                }
            } else
            {
                //std::cout << "CHECK PING: " << neighbour->first << std::endl;
                //std::cout << "  WAS: " << neighbour->second.num_replies.load() << std::endl;
                neighbour->second.num_replies.store(0);
                //std::cout << "  NOW: " << neighbour->second.num_replies.load() << std::endl;
                neighbour++;
            }
        }
    }
    // map_.PrintRouteMap();
    for (auto interface : interfaces_)
    {
        network_v4 interface_net = make_network_v4(interface.first);
        udp::endpoint local_broadcast_ep{
            interface_net.broadcast(),
            port_
        };
        PingByIP(local_broadcast_ep);   
    }
}

/* ================= Root peers methods =============== */

void Peer::SetRoots(std::list<boost::asio::ip::address> &remote_addresses)
{
    for (boost::asio::ip::address addr : remote_addresses)
    {
        roots_.push_back(udp::endpoint{addr, port_});
    }
}

const std::list<udp::endpoint> &Peer::GetRoots()
{
    return roots_;
}