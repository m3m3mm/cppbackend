#include "app_serializer.h"

namespace serialization {
//_________ApplicationStateRepr_________
ApplicationStateRepr::ApplicationStateRepr(const app::ApplicationState& app_state) 
    : players_data_(app_state.players) {

    for(const auto& session : app_state.sessions) {
        game_sessions_.push_back(GameSessionRepr(*session));
    }   
}

ApplicationStateRepr::PlayersData ApplicationStateRepr::RestorePlayersData() {
    return players_data_;
}

ApplicationStateRepr::GameSessionsRepr ApplicationStateRepr::RestoreGameSessionsRepr() {
    return game_sessions_;
}

}//namespace serialization