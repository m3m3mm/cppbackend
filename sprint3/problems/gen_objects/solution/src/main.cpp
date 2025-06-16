#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <memory>
#include <thread>

#include "application.h"
#include "api_handler.h"
#include "command_handler.h"
#include "game_properties.h"
#include "json_constructor.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"
#include "sdk.h"

namespace net = boost::asio;
namespace sys = http_server::sys;

using namespace std::literals;
using namespace http_handler;

namespace {
// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}
}  // namespace

int main(int argc, const char* argv[]) {
    logger::InitBoostLogFilter();

    try {
         if (auto args = command_handler::HandleCommands(argc, argv)) {
            // 1. Загружаем карту из файла и построить модель игры
            extra_data::LootTypes loot_types;
            model::Game game = json_loader::LoadGame(args->config_file, loot_types, args->randomize_spawn_point);

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            // Подписываемся на сигналы и при их получении завершаем работу сервера
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                }
            });

            // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            auto api_strand = net::make_strand(ioc);
            auto api_handler = http_handler::BuilderApiHandler().SetGame(std::move(game))
                                                                .SetLootTypes(std::move(loot_types))
                                                                .SetStrand(std::move(api_strand))
                                                                .SetTimer(std::move(std::chrono::milliseconds(args->tick_period)))
                                                                .Build();

            auto handler = std::make_shared<http_handler::RequestHandler>(std::move(api_handler), 
                                                                          std::move(args->static_dir));

            // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            logger::LogExecution(json_constructor::MakeLogStartJSON(port, address.to_string()), "server started");

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });

            logger::LogExecution(json_constructor::MakeLogStopJSON(EXIT_SUCCESS), "server exited");
        }
    } catch (const std::exception& ex) {
        logger::LogExecution(json_constructor::MakeLogStopJSON(EXIT_FAILURE, ex.what()), "server exited");
        return EXIT_FAILURE;
    }

    return 0;
}
