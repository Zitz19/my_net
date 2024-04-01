#include <iostream>
#include <string>
#include <array>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

constexpr std::uint16_t port = 9012;

class Peer
{
private:
    std::size_t max_message_size_ = 128;
    std::array<char, 128> receiving_buffer_;
    std::string name_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint endpoint_;
    boost::asio::ip::udp::endpoint remote_endpoint_;
public:
    Peer(
        boost::asio::io_context& io_context,
        const boost::asio::ip::address& addr,
        const std::string& name)
        : socket_{io_context}
        , endpoint_{addr, port}
        , name_{name}
        {
            boost::asio::ip::udp::endpoint listen_endpoint{addr, port};
            socket_.open(listen_endpoint.protocol());
            socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            socket_.bind(listen_endpoint);
        }

        void do_receive()
        {
            socket_.async_receive_from(
                boost::asio::buffer(receiving_buffer_), 
                remote_endpoint_,
                [this](const boost::system::error_code& error_code,
                std::size_t bytes_received)
                {
                    if (!error_code.failed() && bytes_received > 0)
                    {
                        std::string received_message{
                            receiving_buffer_.begin(),
                            receiving_buffer_.begin() + bytes_received
                        };
                        if (received_message.find(name_) != 0)
                        {
                            std::cout.write(receiving_buffer_.data(), bytes_received);
                            std::cout << '\n' << std::flush;
                        }
                        do_receive();
                    }
                });
        }

        void do_send()
        {
            std::string name = name_;
            std::string message;
            std::getline(std::cin, message);
            std::string buffer = name.append(": " + message);
            socket_.async_send_to(
                boost::asio::buffer(buffer, max_message_size_),
                remote_endpoint_,
                [this, message](
                    const boost::system::error_code& ,
                    std::size_t bytes_sent)
            {
                std::cout << "You: " << message << '\n';
                do_send();
            });
        }

        void set_remote_endpoint(boost::asio::ip::udp::endpoint remote_endpoint)
        {
            remote_endpoint_ = remote_endpoint;
        }
};

int main(int argc, char* argv[])
{
    std::cout << "Hello from StudNet Server!\n";

    boost::asio::thread_pool thread_pool(2);

    if (argc != 5)
    {
        std::cerr << "Usage: ./StudNet <your_nickname> <your_address> <remote_address_1> <remote_address_2>" << std::endl;
        std::exit(1);
    }

    boost::asio::io_context io_context;
    boost::asio::ip::address addr{boost::asio::ip::make_address(argv[2])};
    boost::asio::ip::address remote_address_1{boost::asio::ip::make_address(argv[3])};
    boost::asio::ip::address remote_address_2{boost::asio::ip::make_address(argv[4])};
    Peer peer_1(io_context, addr, argv[1]);
    Peer peer_2(io_context, addr, argv[1]);
    peer_1.set_remote_endpoint(boost::asio::ip::udp::endpoint{remote_address_1, port});
    peer_2.set_remote_endpoint(boost::asio::ip::udp::endpoint{remote_address_2, port});

    boost::asio::post(thread_pool, [&]
    {
        peer_1.do_receive();
        peer_2.do_receive();
        io_context.run();
    });
    boost::asio::post(thread_pool, [&]
    {
        peer_1.do_send();
        peer_2.do_send();
        io_context.run();
    });

    thread_pool.join();

    return 0;
}