#pragma once

#include <boost/serialization/vector.hpp>

#include <chrono>

#include "../../model/detail/model_serializer.h"
#include "../application.h"
#include "../player_properties.h"

namespace player {
    template<typename Archive>
    void serialize(Archive& ar, PlayerData& player_data, [[maybe_unused]] const unsigned version) {
        ar& player_data.map_id;
        ar& player_data.token;
        ar& player_data.player_id;
    }
}//namespace player

namespace serialization {
class ApplicationStateRepr {
public:
    using GameSessionsRepr = std::vector<serialization::GameSessionRepr>;
    using PlayersData = player::PlayersController::PlayersData;
    
    ApplicationStateRepr() = default;
    explicit ApplicationStateRepr(const app::ApplicationState& app_state);

    [[nodiscard]] PlayersData RestorePlayersData();

    [[nodiscard]] GameSessionsRepr RestoreGameSessionsRepr();

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_data_;
        ar& game_sessions_;
    }
private:
    PlayersData players_data_;
    GameSessionsRepr game_sessions_; 
};
}//namespace serialization