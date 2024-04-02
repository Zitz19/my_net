#include <iostream>
#include <string>
#include <array>
#include <list>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

#include "config/config.h"

class Peer
{
private:
    std::size_t max_message_size_ = 128;
    std::array<char, 128> receiving_buffer_;
    std::string name_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint endpoint_;
    std::list<boost::asio::ip::udp::endpoint> remote_endpoints_;
public:
    Peer(
        boost::asio::io_context& io_context,
        Config net_config)
        : socket_{io_context}
        , endpoint_{boost::asio::ip::make_address(net_config.ip_), net_config.port_}
        , name_{net_config.name_}
        {
            boost::asio::ip::udp::endpoint listen_endpoint{boost::asio::ip::address_v4::any(), net_config.port_};
            socket_.open(listen_endpoint.protocol());
            socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            socket_.bind(listen_endpoint);
            for (auto& elem : net_config.roots_)
            {
                boost::asio::ip::address remote_address{boost::asio::ip::make_address(elem)};
                boost::asio::ip::udp::endpoint remote_endpoint{remote_address, net_config.port_};
                remote_endpoints_.push_back(std::move(remote_endpoint));
            }
        }

        void do_receive()
        {
            socket_.async_receive(
                boost::asio::buffer(receiving_buffer_), 
                [this](const boost::system::error_code& error_code,
                std::size_t bytes_received)
                {
                    if (!error_code.failed() && bytes_received > 0)
                    {
                        std::string received_message{
                            receiving_buffer_.begin(),
                            receiving_buffer_.begin() + bytes_received
                        };
                        if (received_message.find("broadcast") != std::string::npos)
                        { 
                            std::cout.write("INFO: broadcast message was received", 36);
                            std::cout << '\n' << std::flush;
                        }
                        else if (received_message.find(name_) == std::string::npos)
                        {
                            std::cout.write(receiving_buffer_.data(), bytes_received);
                            std::cout << '\n' << std::flush;
                        }
                        do_receive();
                    }
                }
            );
        }

        void stop()
        {
            socket_.cancel();
        }

        void do_send()
        {
            bool is_end = false;
            std::string name = name_;
            std::string message = "<connected>";
            std::string buffer = name + ": " + message;
            for (auto& remote_endpoint : remote_endpoints_)
            {
                socket_.send_to(
                    boost::asio::buffer(buffer, max_message_size_),
                    remote_endpoint
                );
            }
            message.clear();
            while (std::getline(std::cin, message))
            {
                buffer.clear();
                if (message == "-exit")
                {
                    message = "<disconnected>";
                    is_end = true;
                }
                buffer = name + ": " + message;
                for (auto& remote_endpoint : remote_endpoints_)
                {
                    socket_.send_to(
                        boost::asio::buffer(buffer, max_message_size_),
                        remote_endpoint
                    );
                }
                if (is_end)
                {
                    return;
                }
                std::cout << "You: " << message << '\n';
            }
        }

        void add_remote_endpoint(boost::asio::ip::udp::endpoint& remote_endpoint)
        {
            remote_endpoints_.push_back(std::move(remote_endpoint));
        }

        void send_broadcast()
        {
            socket_.set_option(boost::asio::socket_base::broadcast(true));
            boost::asio::ip::udp::endpoint broadcastEnpoint(boost::asio::ip::address_v4::broadcast(), 9012);
            socket_.send_to(boost::asio::buffer("<broadcast message>", max_message_size_), broadcastEnpoint);
            socket_.close();
        }
};

int main(int argc, char* argv[])
{
    std::cout << "Hello from StudNet Server!\n";

    boost::asio::thread_pool thread_pool(2);

    Config net_config = Config::ParseConfig("config.json");

    boost::asio::io_context io_context;
    Peer peer(io_context, net_config);
    
    boost::asio::post(thread_pool, [&]
    {
        peer.do_receive();
        io_context.run();
    });

    boost::system::error_code error;
    boost::asio::ip::udp::socket broadcast_socket{io_context};

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
            boost::asio::post(thread_pool, [&]
            {
                peer.stop();
            });
            thread_pool.stop();
            thread_pool.join();
            return 0;
        case 1:
            peer.do_send();
            break;
        case 2:
            std::cout << "Scanning on neighbours...\n";
            broadcast_socket.open(boost::asio::ip::udp::v4(), error);
            if (!error)
            {
                broadcast_socket.set_option(boost::asio::socket_base::broadcast(true));
                boost::asio::ip::udp::endpoint broadcastEnpoint(boost::asio::ip::address_v4::broadcast(boost::asio::ip::address_v4::from_string(net_config.ip_), 
                                                                                                       boost::asio::ip::address_v4::from_string("255.255.255.0")), 
                                                                9012);
                broadcast_socket.send_to(boost::asio::buffer("<broadcast message>", 128), broadcastEnpoint);
                broadcast_socket.close();
            } else
            {
                std::cout << error.what();
            }
            break;
        default:
            break;
        }
    }
}