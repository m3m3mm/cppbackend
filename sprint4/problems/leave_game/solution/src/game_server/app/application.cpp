#include <stdexcept>

#include "application.h"

namespace app {
using namespace json_constructor;
using namespace model;
using namespace player;
using namespace targets_storage;

using std::optional;
using std::string;
using std::string_view;

const static string_view AUTHORIZATION = "Authorization";
const static string_view CONTENT_TYPE = "Content-Type";

Application::Application(postgres::DatabaseConfig&& config)
    : db_(std::make_unique<postgres::Database>(std::move(config)))
    , use_cases_(std::make_unique<app_database::UseCasesImpl>(db_->GetUnitOfWorkFactory())) {
}

ResponseInfo Application::JoinGame(const string& req_body) {
    auto joining_info = json_loader::LoadJoiningInfo(req_body);
    if(!joining_info) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_JOIN_PARSE_MESSAGE)};
    } else if(joining_info->user_name.empty()) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_NAME_MESSAGE)};    
        
    } else if(!game_->FindMap(joining_info->map_id)) {
        return {http::status::not_found,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_MAP_CODE,
                                  TargetErrorMessage::ERROR_SEARCH_MAP_MESSAGE,
                                  *joining_info->map_id)};
    }

    AuthorizationInfo info = ProcessJoinGame(*joining_info);
    return {http::status::ok,
            MakeBodyJSON(info)};
}

void Application::JoinGame(const player::PlayersController::PlayersData& players) {
    for(const auto& player : players) {
        auto unit_parameters = game_->PrepareUnitParameters(model::Map::Id(player.map_id), player.player_id);

        players_->AddPlayer(player::Token(player.token), unit_parameters);
    }
}

ResponseInfo Application::UpdateState(const Header& header, const string& req_body) {
    if(auto content = FindHeader(header, CONTENT_TYPE); !content.has_value()
                                                        && *content != ContentType::APPLICATION_JSON) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_CONTENT_MESSAGE)};                                                 
    }

    return ExecuteAuthorized(header, [&req_body, this](const string& token) {
        if(auto player = players_->FindPlayerByToken(Token(token))) { 
            if(auto dir = json_loader::LoadUpdateInfo(req_body)) {
                player.value()->UpdateStateDog(*dir);
            } else {
                return ResponseInfo {http::status::bad_request,
                                     MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                                       TargetErrorMessage::ERROR_INVALID_ACTION_PARSE_MESSAGE)};
            }

            return ResponseInfo {http::status::ok,
                                 MakeBodyEmptyObject()};
        }

        return ResponseInfo{http::status::unauthorized,
                            MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_TOKEN_CODE,
                                              TargetErrorMessage::ERROR_SEARCH_TOKEN_MESSAGE)};
    });
}

ResponseInfo Application::ProcessTickActions(const Header& header, const string& req_body) {
    if(auto content = FindHeader(header, CONTENT_TYPE); !content.has_value()
                                                        && *content != ContentType::APPLICATION_JSON) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_CONTENT_MESSAGE)};                                                 
    }

    if(auto time = json_loader::LoadTickInfo(req_body)) {
        auto time_delta = std::chrono::milliseconds(time.value());
        ProcessTickActions(time_delta);
    } else {
        return ResponseInfo {http::status::bad_request,
                             MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                               TargetErrorMessage::ERROR_INVALID_TICK_PARSE_MESSAGE)};
    }

    return ResponseInfo {http::status::ok,
                         MakeBodyEmptyObject()};
}

void Application::ProcessTickActions(const std::chrono::milliseconds& delta) {
    game_->ProcessTickActions(delta);
    auto retirement_players = players_->SendIntoRetirement(game_->GetRetirementTime());

    if(!retirement_players.empty()) {
        SetRecords(retirement_players);
    }

    if(listener_) {
        listener_->OnTick(delta);
    }
}

const player::Player* Application::FindPlayerByToken(const player::Token& token) const {
    return players_->FindPlayerByToken(token).value();
}

void Application::SetGame(model::Game&& game)  {
    game_ = std::make_unique<model::Game>(std::forward<model::Game>(game));
}

void Application::SetListener(std::unique_ptr<ApplicationListener> listener) {
    listener_ = std::move(listener);
}

ResponseInfo Application::GetStaticObjectsInfo(TargetRequestType req_type, 
                                               const string& req_obj,
                                               const extra_data::LootTypes& types) const {
    ResponseInfo resp_info;

    if(req_obj.empty()) {
        resp_info.status = http::status::ok;
        resp_info.body = MakeBodyJSON(req_type, *game_);

        return resp_info;
    }

    if(!(game_->FindMap(Map::Id{req_obj}))) {
        resp_info.status = http::status::not_found;
    } else {
        resp_info.status = http::status::ok;
    }
    resp_info.body = MakeBodyJSON(req_type, *game_, req_obj, types.GetTypes(Map::Id(req_obj)));

    return resp_info;
}

ResponseInfo Application::GetPlayersReqInfo(TargetRequestType req_type, const Header& header) const {
    return ExecuteAuthorized(header, [req_type, this](const string& token) {
        if(auto player = players_->FindPlayerByToken(Token(token))) {
            const auto session = player.value()->GetGameSession();

            switch (req_type) {
                case TargetRequestType::GET_PLAYERS : {
                    auto dogs = session.GetDogs();
                    return ResponseInfo {http::status::ok,
                                         MakeBodyJSON(dogs)};   
                }

                case TargetRequestType::GET_STATE : {
                    auto state = model::GameState{session.GetDogs(), session.GetLoot()};
                    return ResponseInfo {http::status::ok,
                                         MakeBodyJSON(state)};
                } 
            
                default:
                    return ResponseInfo{http::status::bad_request,
                                        MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                        TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE)};
            }      
        }

        return ResponseInfo{http::status::unauthorized,
                            MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_TOKEN_CODE,
                                              TargetErrorMessage::ERROR_SEARCH_TOKEN_MESSAGE)};
    });
}

ResponseInfo Application::GetPlayersRecordList(PlayerRecordReqConfig&& config) {
    if(config.max_items > app::MAX_ITEMS) {
        return ResponseInfo{http::status::bad_request,
                            MakeBodyErrorJSON(TargetErrorCode::ERROR_BAD_REQUEST_CODE,
                                              TargetErrorMessage::ERROR_BAD_REQUEST_MESSAGE)};
    }

    std::vector<PlayerRecord> records = use_cases_->GetPlayersRecordList(config.start, config.max_items);
    
    return ResponseInfo{http::status::ok, MakeBodyJSON(records)};
}

ApplicationState Application::GetApplicationState() const {
    return {game_->GetSessions(), players_->GetPlayersData()};
}

player::PlayersController::PlayersData Application::GetPlayersData() const {
    return players_->GetPlayersData();
}

std::chrono::milliseconds Application::GetRetirementTime() const {
    return game_->GetRetirementTime();
}

AuthorizationInfo Application::ProcessJoinGame(const JoiningInfo& info) {
    UnitParameters parameters = game_->PrepareUnitParameters(info.map_id, info.user_name);
    return players_->AddPlayer(parameters);
}

optional<string> Application::TryExtractToken(const Header& header) const {
    auto value = FindHeader(header, AUTHORIZATION);
    if(!value) {
        return std::nullopt;
    }

    auto token = players_->ValidateToken(*value);
    return token;
}

optional<string> Application::FindHeader(const Header& header, string_view name_header) const {
    auto field_iter = header.find(name_header);

    return field_iter == header.end() ? std::nullopt
                                      : std::make_optional<string>(field_iter->value());
}

void Application::SetRecords(const std::vector<player::PlayerRecord>& records) {
    use_cases_->AddPlayerRecord(records);
}
} // namespace app