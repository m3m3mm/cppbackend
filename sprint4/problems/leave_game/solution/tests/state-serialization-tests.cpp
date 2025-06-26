#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <stdexcept>
#include <iostream>

#include "../src/game_server/app/detail/app_serializer.h"
#include "../src/game_server/app/application.h"
#include "../src/game_server/app/player_properties.h"
#include "../src/game_server/json/json_loader.h"
#include "../src/game_server/model/dynamic_object_properties.h"
#include "../src/game_server/model/detail/model_serializer.h"
#include "../src/game_server/server/extra_data.h"

using namespace model;
using namespace player;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}// namespace

SCENARIO_METHOD(Fixture, "CoordObject serialization"s) {
    GIVEN("A CoordObject"s) {
        const CoordObject coord{21, 53};
        WHEN("coord is serialized"s) {
            output_archive << coord;

            THEN("it is equal to coord after serialization"s) {
                InputArchive input_archive{strm};
                CoordObject restored_coord;
                input_archive >> restored_coord;
                CHECK(coord == restored_coord);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "SpeedUnit serialization"s) {
    GIVEN("A SpeedUnit"s) {
        const SpeedUnit speed{0.0, 4.5};
        WHEN("speed is serialized"s) {
            output_archive << speed;

            THEN("it is equal to speed after serialization"s) {
                InputArchive input_archive{strm};
                SpeedUnit restored_speed;
                input_archive >> restored_speed;
                CHECK(speed == restored_speed);
            }
        }
    }
}



SCENARIO_METHOD(Fixture, "Loot serialization"s) {
    GIVEN("A Loot"s) {
        const Loot loot(0, 1, 10, {1.2, 5.3}, true);

        WHEN("loot is serialized"s) {
            {
                serialization::LootRepr repr(loot);
                output_archive << repr;
            }

            THEN("it can be deserialized"s) {
                InputArchive input_archive{strm};
                serialization::LootRepr repr;

                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(loot.GetId() == restored.GetId());
                CHECK(loot.GetType() == restored.GetType());
                CHECK(loot.GetCost() == restored.GetCost());
                CHECK(loot.GetPosition() == restored.GetPosition());
                CHECK(loot.IsPickedUp() == restored.IsPickedUp());
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog serialization"s) {
    GIVEN("A dog"s) {
        const auto dog = [] {
            const Loot loot(0, 1, 10, {1.2, 5.3});

            Dog dog{{24.1, 30.4}, "Pluto"s, 1, 3};
            dog.AddScore(52);
            dog.UpdateState({0.0, 4.5}, model::Direction::R);
            CHECK(dog.AddLoot(loot));

            return dog;
        }();

        WHEN("dog is serialized"s) {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized"s) {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog.GetDirection() == restored.GetDirection());
                CHECK(dog.GetCoord() == restored.GetCoord());
                CHECK(dog.GetPrevCoord() == dog.GetPrevCoord());
                CHECK(dog.GetBag() == dog.GetBag());
                CHECK(dog.GetBagCapacity() == dog.GetBagCapacity());
                CHECK(dog.GetSpeed() == restored.GetSpeed());
                CHECK(dog.GetScore() == restored.GetScore());
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "GameSession serialization"s) {
    extra_data::LootTypes types;
    auto game = json_loader::LoadGame("../tests/test_data/config _with_capacity.json"s, types, true);
    GIVEN("A GameSession"s) {
        auto session = game.AddSession(model::Map::Id("map1"));
        session->AddDog("Pluto"s, true);
        session->AddDog("Tom"s, true);
        session->GenerateLoot(14000ms);

        WHEN("session is serialized"s) {
            {
                serialization::GameSessionRepr repr{*session};
                output_archive << repr;
            }
            extra_data::LootTypes rest_types;
            auto rest_game = json_loader::LoadGame("../tests/test_data/config _with_capacity.json"s, rest_types, true);
            THEN("it can be deserialized"s) {
                InputArchive input_archive{strm};
                serialization::GameSessionRepr repr;
                input_archive >> repr;

                const auto map_id = repr.RestoreMapId();
                auto rest_session = rest_game.AddSession(map_id);
                CHECK(game.GetSessions().size() == rest_game.GetSessions().size());

                rest_session->AddDogs(repr.RestoreDogs());
                rest_session->AddLostObjects(repr.RestoreLoot());
                CHECK(session->GetDogs().size() == rest_session->GetDogs().size());
                CHECK(session->GetLoot().size() == rest_session->GetLoot().size());
                CHECK(session->GetMapId() == rest_session->GetMapId());

                //Проверяем корректность присвоения id. Было 2 собаки с id 0 и 1 
                const auto dog = rest_session->AddDog("Pluto"s, true);
                CHECK(dog->GetId() == 2);

                //Проверяем корректность присвоения id. Было 2 единицы лута с id 0 и 1
                rest_session->GenerateLoot(10000ms);
                CHECK(rest_session->GetLoot().back().GetId() == 2); 
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "PlayerData serialization"s) {
    GIVEN("A PlayerData"s) {
        PlayerData data{"map1"s, "6a180993f43ca9823261c390d71ef332"s, 0};
        WHEN("data is serialized"s) {
            output_archive << data;

            THEN("it is equal to data after serialization"s) {
                InputArchive input_archive{strm};
                player::PlayerData restored_data;
                input_archive >> restored_data;
                CHECK(data == restored_data);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "ApplicationState serialization"s) {
    app::Application application;
    extra_data::LootTypes types;
    application.SetGame(json_loader::LoadGame("../tests/test_data/config _with_capacity.json"s, types, true));
    application.JoinGame("{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"}"s);
    application.JoinGame("{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"}"s);

    GIVEN("A ApplicationData"s) {
        WHEN("state is serialized"s) {
            auto state = application.GetApplicationState();
            {
                serialization::ApplicationStateRepr repr(state) ;
                output_archive << repr;
            }
        
            THEN("it can be deserialized"s) {
                InputArchive input_archive{strm};
                serialization::ApplicationStateRepr rest_state;
                input_archive >> rest_state;

                auto rest_sessions = rest_state.RestoreGameSessionsRepr();
                CHECK(rest_sessions.size() == state.sessions.size());
                CHECK(rest_sessions.front().RestoreMapId() == state.sessions.front()->GetMapId());
                CHECK(rest_sessions.front().RestoreDogs().size() == state.sessions.front()->GetDogs().size());
                CHECK(rest_sessions.front().RestoreLoot().size() == state.sessions.front()->GetLoot().size());

                auto rest_players = rest_state.RestorePlayersData();
                CHECK(rest_players.size() == state.players.size());
                CHECK(rest_players == state.players);

                auto player = application.FindPlayerByToken(Token(rest_players.front().token));
                CHECK(player);

                app::Application rest_application;
                model::Game game = json_loader::LoadGame("../tests/test_data/config _with_capacity.json"s, types, true);

                auto session = game.AddSession(rest_sessions.front().RestoreMapId());
                session->AddDogs(rest_sessions.front().RestoreDogs());
                session->AddLostObjects(rest_sessions.front().RestoreLoot());
                
                rest_application.SetGame(std::move(game));
                rest_application.JoinGame(rest_players);
                
                auto rest_player = rest_application.FindPlayerByToken(Token(rest_players.front().token));
                CHECK(rest_player->GetDogId() == player->GetDogId());
                CHECK(rest_player->GetMapId() == player->GetMapId());
            }
        }
    }
}
