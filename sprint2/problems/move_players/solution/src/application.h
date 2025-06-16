#pragma once

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <memory>
#include <optional>
#include <string>

#include "json_constructor.h"
#include "json_loader.h"
#include "model.h"
#include "player_properties.h"
#include "target_storage.h"

namespace app {
namespace beast = boost::beast;
namespace http = beast::http;

using Header = http::header<true, beast::http::fields>;

struct ResponseInfo {
    http::status status;
    std::string body;  
};

class Application {
public:
    Application(model::Game&& game)
        : game_(std::make_unique<model::Game>(std::forward<model::Game>(game)))
        , players_(std::make_unique<player::Players>()) {
    }

    ResponseInfo JoinGame(const std::string& req_body);
    ResponseInfo MoveUnit(const Header& header, const std::string& req_body);

    ResponseInfo GetStaticObjectsInfo(targets_storage::TargetRequestType req_type, std::string& req_obj) const;
    ResponseInfo GetPlayersReqInfo(targets_storage::TargetRequestType req_type, const Header& header) const;
private:
    std::unique_ptr<model::Game> game_;
    std::unique_ptr<player::Players> players_;

    void ProcessMoveUnit(const player::Player& player, model::Direction dir);
    player::AuthorizationInfo ProcessJoinGame(const player::JoiningInfo& info);
    std::optional<std::string> TryExtractToken(const Header& header) const;
    std::optional<std::string> FindHeader(const Header& header,const std::string& name_header) const;


    template <typename Fn>
    ResponseInfo ExecuteAuthorized(const Header& header, Fn&& action) const {
        using namespace targets_storage;
        using namespace json_constructor;

        if (auto token = TryExtractToken(header)) {
            return action(*token);
        } else {
            return ResponseInfo{http::status::unauthorized,
                                MakeBodyErrorJSON(TargetErrorCode::ERROR_INVALID_TOKEN_CODE,
                                                  TargetErrorMessage::ERROR_INVALID_TOKEN_MESSAGE)};
        }
    }
};
}//namespace app