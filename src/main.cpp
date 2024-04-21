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
            client.Stop();
            return 0;
        case 1:
            client.Send();
            break;
        case 2:
            std::cout << "Scanning on neighbours...\n";
            client.SearchNeighbours();
            break;
        default:
            break;
        }
    }

    return 0;
}