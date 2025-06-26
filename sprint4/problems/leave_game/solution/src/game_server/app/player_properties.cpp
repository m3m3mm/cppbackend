#include <regex>
#include <set>

#include "player_properties.h"

namespace player {
const static std::string_view BEARER = "Bearer";
const static std::string_view TOKEN_REG = "^[a-fA-F0-9]{32}$";
const static double DEF_SPEED = 0.0;
const static int SIZE_TOKEN = 32;
    
using namespace model;

using std::string;

//__________Player__________
Player::Player(const model::UnitParameters& parameters)
    : session_(parameters.session)
    , dog_(parameters.dog){
}

void Player::UpdateStateDog(Direction dir) const {
    double speed_on_map = session_->GetMap().GetSpeed();

    switch (dir) {
        case Direction::U :
            dog_->UpdateState({DEF_SPEED, -speed_on_map}, dir);       
            break;

        case Direction::D :
            dog_->UpdateState({DEF_SPEED, speed_on_map}, dir); 
            break;

        case Direction::L :
            dog_->UpdateState({-speed_on_map, DEF_SPEED}, dir); 
            break;

        case Direction::R :
            dog_->UpdateState({speed_on_map, DEF_SPEED}, dir);
            break;

        default:
            dog_->UpdateState({DEF_SPEED, DEF_SPEED}, dog_->GetDirection());
            break;
    }

    if(dog_->GetDirection() != Direction::STOP) {
        dog_->SetInactiveTime(std::chrono::milliseconds(0));
    }
}

void Player::RetireDog(size_t dog_id) {
    session_->DeleteDog(dog_id);
    is_retirement_ = true;
}

const GameSession& Player::GetGameSession() const {
    return *session_;
}

const Dog* Player::GetDog() const {
    return dog_.get();
}

Map::Id Player::GetMapId() const {
    return session_->GetMapId();
}

size_t Player::GetDogId() const {
    return dog_->GetId();
}

bool Player::IsRetirement() const {
    return is_retirement_;
}

//__________PlayersController__________
AuthorizationInfo PlayersController::AddPlayer(const UnitParameters& parameters) {
    Token token(GenerateToken());

    Player player(parameters);
    token_to_players_.insert({token, player});

    return AuthorizationInfo{token, player.GetDogId()};
}

void PlayersController::AddPlayer(Token token, const UnitParameters& parameters) {
    token_to_players_.insert({token, Player(parameters)});
}

std::optional<string> PlayersController::ValidateToken(const string& value_token) const {
    static std::regex token_reg(TOKEN_REG.data());
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

std::vector<PlayerRecord> PlayersController::SendIntoRetirement(const std::chrono::milliseconds& retirement_time) {
    std::vector<PlayerRecord> retirement_palyers;

    for(auto& [_, player] : token_to_players_) {
        auto dog = player.GetDog();
        if(dog->GetInactiveTime() >= retirement_time) {
            retirement_palyers.push_back({dog->GetName(), dog->GetTimeInGame(), dog->GetScore()});
            player.RetireDog(dog->GetId());
        }
    }

    //Безопасно удаляем игроков-пенсионеров
    std::erase_if(token_to_players_, [](const auto& item) {
        return item.second.IsRetirement();
    });

    return retirement_palyers;
}

std::optional<const Player*> PlayersController::FindPlayerByToken(const Token& token) const {
    auto iter = token_to_players_.find(token);

    if(iter == token_to_players_.end()) {
        return std::nullopt;
    }

    return &iter->second;
}

std::vector<PlayerData> PlayersController::GetPlayersData() const {
    std::vector<PlayerData> result;

    for(const auto& [token, player] : token_to_players_) {
        result.push_back(PlayerData{*player.GetMapId(), *token, player.GetDogId()});
    }

    return result;
}

string PlayersController::GenerateToken() {
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