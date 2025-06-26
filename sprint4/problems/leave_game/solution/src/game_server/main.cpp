#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <memory>
#include <thread>

#include "../database/postgres/postgres.h"
#include "../database/app/unit_of_work.h"
#include "app/application.h"
#include "handlers/api_handler.h"
#include "handlers/command_handler.h"
#include "handlers/request_handler.h"
#include "json/json_constructor.h"
#include "json/json_loader.h"
#include "model/game_properties.h"
#include "server/logger.h"
#include "sdk.h"

namespace net = boost::asio;
namespace sys = http_server::sys;

using namespace std::literals;
using namespace http_handler;

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

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
            
            const unsigned num_threads = std::thread::hardware_concurrency();

            //2. Загружаем переменную окружения, и создаем конфигурацию для базы данных
            const auto db_url = std::getenv(DB_URL_ENV_NAME);
            if (!db_url) {
                throw std::runtime_error("Cannot read database URL");
            } 
            postgres::DatabaseConfig config{num_threads, db_url};
            
            // 3. Инициализируем io_context
            net::io_context ioc(num_threads);

            // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            // Подписываемся на сигналы и при их получении завершаем работу сервера
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                }
            });

            // 5. Создаём обработчик HTTP-запросов и связываем его с моделью игры
            auto api_strand = net::make_strand(ioc);
            auto api_handler = http_handler::BuilderApiHandler().SetGame(std::move(game))
                                                                .SetLootTypes(std::move(loot_types))
                                                                .SetStrand(std::move(api_strand))
                                                                .SetTimer(std::move(std::chrono::milliseconds(args->tick_period)))
                                                                .SetStateFile(std::move(args->state_file))
                                                                .SetDatabaseConfig(std::move(config))
                                                                .Build();

            auto handler = std::make_shared<http_handler::RequestHandler>(std::move(args->static_dir), 
                                                                          std::move(api_handler),
                                                                          std::move(args->save_state_period));

            // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            logger::LogExecution(json_constructor::MakeLogStartJSON(port, address.to_string()), "server started");

            // 7. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });

            handler->SaveState();
            logger::LogExecution(json_constructor::MakeLogStopJSON(EXIT_SUCCESS), "server exited");
        }
    } catch (const std::exception& ex) {
        logger::LogExecution(json_constructor::MakeLogStopJSON(EXIT_FAILURE, ex.what()), "server exited");
        return EXIT_FAILURE;
    }

    return 0;
}
