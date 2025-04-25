#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>

using namespace std::literals;
using boost::asio::ip::udp;

void StartServer(uint16_t port) {
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

    Player player(ma_format_u8, 1);
    const int frame_size = player.GetFrameSize();

    std::vector<char> buffer(65536);

    std::cout << "Server started on port " << port << std::endl;

    while (true) {
        udp::endpoint sender_endpoint;
        boost::system::error_code ec;
        size_t bytes_received = socket.receive_from(boost::asio::buffer(buffer), sender_endpoint, 0, ec);

        if (ec) {
            std::cerr << "Receive error: " << ec.message() << std::endl;
            continue;
        }

        size_t frames = bytes_received / frame_size;
        std::cout << "Received " << bytes_received << " bytes (" << frames << " frames)" << std::endl;

        player.PlayBuffer(buffer.data(), frames, 1.5s);
    }
}

void StartClient(uint16_t port) {
    boost::asio::io_context io_context;
    udp::socket socket(io_context);
    socket.open(udp::v4());

    Recorder recorder(ma_format_u8, 1);
    const int frame_size = recorder.GetFrameSize();

    while (true) {
        std::string ip;
        std::cout << "Enter server IP and press Enter: ";
        std::getline(std::cin, ip);

        std::cout << "Press Enter to record message..." << std::endl;
        std::string dummy;
        std::getline(std::cin, dummy);

        auto result = recorder.Record(65000, 1.5s);
        size_t bytes_to_send = result.frames * frame_size;

        udp::endpoint endpoint(boost::asio::ip::make_address(ip), port);

        boost::system::error_code ec;
        size_t sent = socket.send_to(boost::asio::buffer(result.data.data(), bytes_to_send), endpoint, 0, ec);

        if (ec) {
            std::cerr << "Send error: " << ec.message() << std::endl;
        } else {
            std::cout << "Sent " << sent << " bytes to " << ip << ":" << port << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <client|server> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    try {
        if (mode == "server") {
            StartServer(port);
        } else if (mode == "client") {
            StartClient(port);
        } else {
            std::cerr << "Unknown mode: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

