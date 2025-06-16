#include "player_properties.h"

namespace player {
using namespace model;

//__________Player__________
const GameSession& Player::GetGameSession() const {
    return *session_;
}

Map::Id Player::GetMapId() const {
    return session_->GetMapId();
}

size_t Player::GetDogId() const {
    return dog_->GetId();
}

//__________Players__________
AuthorizationInfo Players::AddPlayer(const UnitParameters& parameters) {
    Token token(GenerateToken());

    const Player* player = &players_.emplace_back(parameters);
    token_to_players_.insert({token, player});

    return AuthorizationInfo{token, player->GetDogId()};
}

std::optional<const Player*> Players::FindPlayerByToken(const Token& token) {
    auto iter = token_to_players_.find(token);

    if(iter == token_to_players_.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::optional<std::string> Players::ValidateToken(const std::string& value_token) {
    static std::regex token_reg("^[a-f0-9]{32}$");
    std::smatch m;

    if(value_token.starts_with("Bearer")) {
        auto pos = value_token.find_last_of(' ');

        if (pos != std::string::npos) {
            std::string token = value_token.substr(pos + 1, 32);
            if(std::regex_match(token, m, token_reg)) {
                return token;
            }
        }
    }

    return std::nullopt;
}
const std::deque<Player>& Players::GetPlayers() const {
    return players_;
}

std::string Players::GenerateToken() {
    std::stringstream buf;

    auto first_part = generator1_();
    auto second_part = generator2_();

    buf << std::hex << first_part << second_part;
    std::string token_string = buf.str();

    while (token_string.size() < 32) {
        token_string.insert(0, "0");
    }

    return  token_string;
}

} // namespace palyer