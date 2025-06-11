#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <string_view>
#include <string>
#include <utility>
#include <unordered_map>

#include "model.h"
#include "game_properties.h"

namespace player {
    namespace detail {
    struct TokenTag {};
    }// namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

struct JoiningInfo{
    std::string user_name;
    model::Map::Id map_id;   
};

struct AuthorizationInfo {
    Token token;
    //В качестве PlayerId используется Id собаки
    size_t player_id;
};

class Player {
public:
    Player(const model::UnitParameters& parameters) 
        : session_(parameters.session)
        , dog_(parameters.dog){
    }

    const model::GameSession& GetGameSession() const;
    model::Map::Id GetMapId() const;

    size_t GetDogId() const;
private:
    std::shared_ptr<model::GameSession> session_;
    std::shared_ptr<model::Dog> dog_;
};

class Players {
public:
    AuthorizationInfo AddPlayer(const model::UnitParameters& parameters);

    std::optional<const Player*> FindPlayerByToken(const Token& token);

    std::optional<std::string> ValidateToken(const std::string& value_token);

    const std::deque<Player>& GetPlayers() const; 
private:
    std::deque<Player> players_;
    std::unordered_map<Token, const Player*, util::TaggedHasher<Token>> token_to_players_;

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    std::string GenerateToken();
};
}//namespace player
