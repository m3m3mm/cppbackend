#pragma once

#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../model/dynamic_object_properties.h"
#include "../model/game_properties.h"
#include "../model/static_object_prorerties.h"
#include "../tagged.h"

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

struct PlayerData {
    std::string map_id = "";
    std::string token = "";
    size_t player_id = 0;

    [[nodiscard]] auto operator<=>(const PlayerData&) const = default;
};

struct PlayerRecord {
    std::string name;
    std::chrono::milliseconds total_time;
    size_t score;
};

class Player {
public:
    explicit Player(const model::UnitParameters& parameters);

    void UpdateStateDog(model::Direction dir) const;
    void RetireDog (size_t dog_id);

    const model::GameSession& GetGameSession() const;
    const model::Dog* GetDog() const;
    model::Map::Id GetMapId() const;
    size_t GetDogId() const;

    bool IsRetirement() const;
private:
    std::shared_ptr<model::GameSession> session_;
    mutable std::shared_ptr<model::Dog> dog_;
    bool is_retirement_ = false;
};

class RetiredPlayersRepository {
public:
    virtual ~RetiredPlayersRepository() = default;
    virtual void Save(const std::vector<player::PlayerRecord>& player_records) = 0;
    virtual std::vector<PlayerRecord> GetPlayersRecordList(size_t offset, size_t limit) const = 0;
};

class PlayersController {
public:
    using PlayersData = std::vector<player::PlayerData>;

    AuthorizationInfo AddPlayer(const model::UnitParameters& parameters);
    void AddPlayer(Token token, const model::UnitParameters& parameters);

    std::optional<std::string> ValidateToken(const std::string& value_token) const;
    std::vector<PlayerRecord> SendIntoRetirement(const std::chrono::milliseconds& retirement_time);

    std::optional<const Player*> FindPlayerByToken(const Token& token) const;

    PlayersData GetPlayersData() const;
private:
    std::unordered_map<Token, Player, util::TaggedHasher<Token>> token_to_players_;

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