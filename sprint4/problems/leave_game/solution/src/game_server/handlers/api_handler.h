#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/url_view.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <memory>

#include "../../database/postgres/postgres.h"
#include "../app/application.h"
#include "../server/extra_data.h"
#include "../model/game_properties.h"
#include "../model/static_object_prorerties.h"
#include "state_handler.h"
#include "target_storage.h"

namespace beast = boost::beast;
namespace fs    = std::filesystem;
namespace http  = beast::http;
namespace net   = boost::asio;
namespace sys   = boost::system;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

using Strand = net::strand<net::io_context::executor_type>;

namespace http_handler {
class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Handler = std::function<void(std::chrono::milliseconds)>;

    Ticker(Strand& strand, const std::chrono::milliseconds& period, Handler handler);

    void Start();
private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

private:
    Strand& strand_;
    net::steady_timer timer_{strand_};
    std::chrono::milliseconds period_;
    Handler handler_;
    std::chrono::time_point<std::chrono::steady_clock> last_tick_;
};

class ApiHandler;

class BuilderApiHandler {
public:
    BuilderApiHandler& SetStrand(Strand&& strand);
    BuilderApiHandler& SetGame(model::Game&& game);
    BuilderApiHandler& SetLootTypes(extra_data::LootTypes&& loot_types);
    BuilderApiHandler& SetTimer(std::chrono::milliseconds&& timer);
    BuilderApiHandler& SetStateFile(fs::path path);
    BuilderApiHandler& SetDatabaseConfig(postgres::DatabaseConfig&& config);

    ApiHandler Build();
private:
    std::unique_ptr<Strand> api_strand_ = nullptr;
    std::unique_ptr<model::Game> game_ = nullptr;
    std::unique_ptr<extra_data::LootTypes> loot_types_ = nullptr;
    std::unique_ptr<std::chrono::milliseconds> timer_ = nullptr;
    std::unique_ptr<fs::path> state_file_ = nullptr;
    std::unique_ptr<postgres::DatabaseConfig> config_ = nullptr;
};

class ApiHandler {
public:
    friend BuilderApiHandler;
    void Start(std::chrono::milliseconds&& save_period);
    StringResponse HanldeApiRequest(const StringRequest& req, targets_storage::TargetRequestType req_type);
    void SaveState() const;

    Strand GetApiStrand() const;
private:
    ApiHandler(Strand&& api_strand, 
               postgres::DatabaseConfig&& config,
               extra_data::LootTypes&& loot_types, 
               std::chrono::milliseconds&& timer,
               std::filesystem::path&& state_file,
               model::Game&& game);

    Strand api_strand_;
    app::Application app_;
    extra_data::LootTypes loot_types_;
    state_handler::StateHandler state_handler_;
    std::shared_ptr<Ticker> ticker_;
    std::chrono::milliseconds timer_;
    bool success_restore_ = false;

    std::string ComputeRequestedObject(std::string_view target) const;
};
}