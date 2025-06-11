#include "application.h"

namespace app{
using namespace json_constructor;
using namespace model;
using namespace player;
using namespace targets_storage;

using std::string;

const static string AUTHORIZATION = "Authorization";

ResponseInfo Application::JoinGame(TargetRequestType req_type, const string& req_body) {
    auto joining_info = json_loader::LoadJoiningInfo(req_body);
    if(!joining_info) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_ARGUMENT_MESSAGE)};
    } else if(joining_info->user_name.empty()) {
        return {http::status::bad_request,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_ARGUMENT_CODE,
                                  TargetErrorMessage::ERROR_INVALID_NAME_MESSAGE)};    
        
    } else if(!game_->FindMap(joining_info->map_id)) {
        return {http::status::not_found,
                MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_MAP_CODE,
                                  TargetErrorMessage::ERROR_SEARCH_MAP_MESSAGE,
                                  *joining_info->map_id)};
    } else {
        AuthorizationInfo info = ProcessJoinGame(*joining_info);
        return {http::status::ok,
                MakeBodyJSON(req_type, info)};
    }
}

ResponseInfo Application::GetStaticObjectsInfo(TargetRequestType req_type, string& req_obj) const {
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
    resp_info.body = MakeBodyJSON(req_type, *game_, req_obj);

    return resp_info;
}

ResponseInfo Application::GetPlayersReqInfo(TargetRequestType req_type, const Header& header) const {
    auto token = ValidateHeader(header);

    if(!token.has_value()) {
        return ResponseInfo{http::status::unauthorized,
                            MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_TOKEN_CODE,
                                              TargetErrorMessage::ERROR_INVALID_TOKEN_MESSAGE)};
    }
    
    if(auto player = players_->FindPlayerByToken(Token(*token))) {
        const auto session = player.value()->GetGameSession();
        const auto dogs = session.GetDogs();

        return {http::status::ok,
                MakeBodyJSON(req_type, dogs)};       
    }

    return {http::status::unauthorized,
            MakeBodyErrorJSON(TargetErrorCode::ERROR_SEARCH_TOKEN_CODE,
                              TargetErrorMessage::ERROR_SEARCH_TOKEN_MESSAGE)};
}

AuthorizationInfo Application::ProcessJoinGame(const JoiningInfo& info) {
    UnitParameters parameters = game_->PrepareUnitParameters(info.map_id, info.user_name);
    return players_->AddPlayer(parameters);
}

std::optional<string> Application::ValidateHeader(const Header& header) const {
    auto field_iter = header.find(AUTHORIZATION);
    if(field_iter == header.end()) {
        return std::nullopt;
    }

    auto token = players_->ValidateToken(field_iter->value().data());
    return std::move(token);
}
} // namespace app