// src/main.cpp
#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging.h"
#include "logging_request_handler.h"
#include "model.h"
#include "http_server.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

using namespace std::literals;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

// Run function fn on n threads, including the current one
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Launch n-1 worker threads
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <config-file> <static-files-dir>" << std::endl;
        return 1;
    }

    try {
        // Initialize logging
        logging::InitLogging();

        // Load game model
        model::Game game = json_loader::LoadGame(argv[1]);
        
        // Create request handler
        http_handler::RequestHandler handler(game, argv[2]);
        
        // Create logging request handler
        LoggingRequestHandler logging_handler(handler);

        // Setup server
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // Setup signal handling
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number) {
            if (!ec) {
                std::cout << "Received signal " << signal_number << ", shutting down..." << std::endl;
                ioc.stop();
            }
        });

        // Create and launch the listening port
        auto address = net::ip::make_address("0.0.0.0");
        unsigned short port = 8080;

        // Log server start
        logging::LogServerStart(port, address.to_string());

        // Create and launch the listening port
        http_server::ServeHttp(ioc, tcp::endpoint(address, port), logging_handler);

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(num_threads - 1);
        for(auto i = num_threads - 1; i > 0; --i)
            v.emplace_back(
                [&ioc]
                {
                    ioc.run();
                });
        ioc.run();

        // Block until all the threads exit
        for(auto& t : v)
            t.join();

        // Log server stop
        logging::LogServerStop(0);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        logging::LogServerStop(1, &e);
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        logging::LogServerStop(1);
        return 1;
    }

    return 0;
}
