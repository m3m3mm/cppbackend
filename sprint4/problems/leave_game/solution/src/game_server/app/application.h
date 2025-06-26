#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../database/app/use_cases_impl.h"
#include "../../database/postgres/postgres.h"
#include "../handlers/target_storage.h"
#include "../json/json_constructor.h"
#include "../json/json_loader.h"
#include "../model/static_object_prorerties.h"
#include "../server/extra_data.h"
#include "player_properties.h"

namespace app {
const static size_t MAX_ITEMS = 100;

namespace beast = boost::beast;
namespace http = beast::http;

using Header = http::header<true, beast::http::fields>;

struct PlayerRecordReqConfig {
    size_t start = 0;
    size_t max_items = 100;
};

struct ResponseInfo {
    http::status status;
    std::string body;  
};

struct ApplicationState {
    std::vector<std::shared_ptr<model::GameSession>> sessions;
    std::vector<player::PlayerData> players;
};

class ApplicationListener {
public:
    virtual ~ApplicationListener() = default;
    virtual void OnTick(const std::chrono::milliseconds& delta) = 0;
};

class Application {
public:
    Application() = default;
    explicit Application(postgres::DatabaseConfig&& config);
    
    ResponseInfo JoinGame(const std::string& req_body);
    void JoinGame(const player::PlayersController::PlayersData& players);

    ResponseInfo UpdateState(const Header& header, const std::string& req_body);
    ResponseInfo ProcessTickActions(const Header& header, const std::string& req_body);
    void ProcessTickActions(const std::chrono::milliseconds& delta);

    const player::Player* FindPlayerByToken(const player::Token& token) const;

    void SetGame(model::Game&& game);
    void SetListener(std::unique_ptr<ApplicationListener> listener);

    ResponseInfo GetStaticObjectsInfo(targets_storage::TargetRequestType req_type, 
                                      const std::string& req_obj,
                                      const extra_data::LootTypes& types) const;
    ResponseInfo GetPlayersReqInfo(targets_storage::TargetRequestType req_type, const Header& header) const;
    ResponseInfo GetPlayersRecordList(PlayerRecordReqConfig&& config);
    ApplicationState GetApplicationState() const;
    player::PlayersController::PlayersData GetPlayersData() const;
    std::chrono::milliseconds GetRetirementTime() const;
private:
    std::unique_ptr<model::Game> game_ = nullptr;
    std::unique_ptr<player::PlayersController> players_ = std::make_unique<player::PlayersController>();
    std::unique_ptr<ApplicationListener> listener_ = nullptr;

    std::unique_ptr<postgres::Database>  db_ = nullptr;
    std::unique_ptr<app_database::UseCasesImpl> use_cases_ = nullptr;
    
    player::AuthorizationInfo ProcessJoinGame(const player::JoiningInfo& info);
    std::optional<std::string> TryExtractToken(const Header& header) const;
    std::optional<std::string> FindHeader(const Header& header,const std::string_view name_header) const;
    void SetRecords(const std::vector<player::PlayerRecord>& records);
    
    template <typename Fn>
    ResponseInfo ExecuteAuthorized(const Header& header, Fn&& action) const {
        using namespace targets_storage;
        using namespace json_constructor;

        if (auto token = TryExtractToken(header)) {
            return action(*token);
        }
        
        return ResponseInfo{http::status::unauthorized,
                            MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_TOKEN_CODE,
                                              TargetErrorMessage::ERROR_INVALID_TOKEN_MESSAGE)};
    }
};
}//namespace app