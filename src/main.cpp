#include <boost/asio.hpp>
#include <iostream>

#include "core/client.h"
#include "config/config.h"

using boost::asio::ip::udp;

int main()
{
    std::cout << "Hello from StudNet!\n";
    Net client{Config::ParseConfig("config.json")};
    client.Receive();

    while (true)
    {
        std::cout << "Choose the option:\n[0]   Exit\n[1]   Send messages to known roots\n[2]   Scan local net on neighbours\n";
        int choice = 999;
        std::cin >> choice;
        std::cin.clear();
        std::cin.ignore(INT_MAX, '\n');
        switch (choice)
        {
        case 0:
            // r.join();
            return 0;
        case 1:
            client.Send();
            break;
        case 2:
            // std::cout << "Scanning on neighbours...\n";
            // broadcast_socket.open(boost::asio::ip::udp::v4(), error);
            // if (!error)
            // {
            //     broadcast_socket.set_option(boost::asio::socket_base::broadcast(true));
            //     boost::asio::ip::udp::endpoint broadcastEnpoint(boost::asio::ip::address_v4::broadcast(boost::asio::ip::address_v4::from_string(net_config.ip_), 
            //                                                                                            boost::asio::ip::address_v4::from_string("255.255.255.0")), 
            //                                                     9012);
            //     broadcast_socket.send_to(boost::asio::buffer("<broadcast message>", 128), broadcastEnpoint);
            //     broadcast_socket.close();
            // } else
            // {
            //     std::cout << error.what();
            // }
            // break;
        default:
            break;
        }
    }

    return 0;
}