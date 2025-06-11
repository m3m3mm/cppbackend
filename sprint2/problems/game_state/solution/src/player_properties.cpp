#include "player_properties.h"

#include <regex>

namespace player {
using namespace model;

using std::string;

const static int SIZE_TOKEN = 32;
const static string BEARER = "Bearer";
const static string TOKEN_REG = "^[a-fA-F0-9]{32}$";

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

std::optional<const Player*> Players::FindPlayerByToken(const Token& token) const {
    auto iter = token_to_players_.find(token);

    if(iter == token_to_players_.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::optional<string> Players::ValidateToken(const string& value_token) const {
    static std::regex token_reg(TOKEN_REG);
    std::smatch m;

    if(value_token.starts_with(BEARER)) {
        auto pos = value_token.find(' ');
        if (pos != string::npos) {
            string token = value_token.substr(pos + 1, SIZE_TOKEN);
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

string Players::GenerateToken() {
    std::stringstream buf;

    auto first_part = generator1_();
    auto second_part = generator2_();

    buf << std::hex << first_part << second_part;
    string token_string = buf.str();

    while (token_string.size() < SIZE_TOKEN) {
        token_string.insert(0, "0");
    }

    return  token_string;
}
} // namespace palyer