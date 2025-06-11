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

    ResponseInfo JoinGame(targets_storage::TargetRequestType req_type, const std::string& req_body);

    ResponseInfo GetStaticObjectsInfo(targets_storage::TargetRequestType req_type, std::string& req_obj) const;
    ResponseInfo GetPlayersReqInfo(targets_storage::TargetRequestType req_type, const Header& header) const;
private:
    std::unique_ptr<model::Game> game_;
    std::unique_ptr<player::Players> players_;

    player::AuthorizationInfo ProcessJoinGame(const player::JoiningInfo& info);
    std::optional<std::string> ValidateHeader(const Header& header) const;
};
}//namespace app